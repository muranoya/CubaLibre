use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream, Shutdown};
use std::time::{Duration};
use std::collections::HashMap;
use std::collections::hash_map::Entry::{Occupied, Vacant};
use std::fmt;
use std::clone;
use std::thread;
use std::env;

#[derive(Clone)]
enum HttpMethod {
    GET,
    POST,
    HEAD,
    OPTIONS,
    PUT,
    DELETE,
    TRACE,
    PATCH,
}

impl fmt::Display for HttpMethod {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", match self {
            &HttpMethod::GET     => "GET",
            &HttpMethod::POST    => "POST",
            &HttpMethod::HEAD    => "HEAD",
            &HttpMethod::OPTIONS => "OPTIONS",
            &HttpMethod::PUT     => "PUT",
            &HttpMethod::DELETE  => "DELETE",
            &HttpMethod::TRACE   => "TRACE",
            &HttpMethod::PATCH   => "PATCH",
        })
    }
}

#[derive(Clone)]
enum HttpVersion {
    Http0_9,
    Http1_0,
    Http1_1,
}

impl fmt::Display for HttpVersion {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", match self {
            &HttpVersion::Http0_9 => "",
            &HttpVersion::Http1_0 => "HTTP/1.0",
            &HttpVersion::Http1_1 => "HTTP/1.1",
        })
    }
}

struct HttpRequest {
    method:  HttpMethod,
    version: HttpVersion,
    uri:     String,
    headers: HashMap<String, String>,
    body:    Vec<u8>,
}

impl fmt::Display for HttpRequest {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let _ = write!(f, "{} {} {}\r\n", self.method, self.uri, self.version);
        for (k, v) in self.headers.iter() {
            let _ = write!(f, "{}: {}\r\n", k, v);
        }
        let _ = write!(f, "\r\n");
        write!(f, "{}", String::from_utf8(self.body.clone()).unwrap())
    }
}

impl clone::Clone for HttpRequest {
    fn clone(&self) -> HttpRequest {
        HttpRequest {
            method:  self.method.clone(),
            version: self.version.clone(),
            uri:     self.uri.clone(),
            headers: self.headers.clone(),
            body:    self.body.clone(),
        }
    }
    fn clone_from(&mut self, src: &HttpRequest) {
        self.method  = src.method.clone();
        self.version = src.version.clone();
        self.uri     = src.uri.clone();
        self.headers = src.headers.clone();
        self.body    = src.body.clone();
    }
}

struct RToption {
    listen_addr: String,
    listen_port: u16,
}

fn print_usage(args:&mut env::Args) {
    println!("Usage: {} [port]", args.nth(0).unwrap());
}

fn set_options(args:&mut env::Args) -> Result<RToption, String> {
    let o = RToption {
        listen_addr: "0.0.0.0".to_string(),
        listen_port: 8080,
    };
    Ok(o)
}

fn sendrecv_data(mut req: HttpRequest, mut sock: TcpStream) -> Result<(), String> {
    let host = match req.headers.entry("Host".to_string()) {
        Occupied(e) => {
            e.get().clone()
        },
        Vacant(_) => {
            return Err("Error: no host".to_string())
        },
    };

    let mut stream = match TcpStream::connect(format!("{}:80", host)) {
        Ok(stream) => stream,
        Err(e)     => return Err(e.to_string()),
    };

    let _ = stream.write_all(format!("{}", req).as_bytes());

    loop {
        let mut buf = [0; 64*1024];
        let readsize = stream.read(&mut buf).unwrap();
        if readsize == 0 {
            sock.shutdown(Shutdown::Both);
            break;
        }
        sock.write_all(&buf[0..readsize]).unwrap();
    }
    Ok(())
}

fn camouflage_client(req: &mut HttpRequest) {
    req.headers.remove("Proxy-Connection");
    /* そのうちなんとかする */
    req.headers.remove("Accept-Encoding");

    req.headers.insert("Cache-Control".to_string(), "max-age=0".to_string());
    req.headers.insert("Connection".to_string(), "keep-alive".to_string());

    const SCHM: &str = "http://";
    if &req.uri[0..SCHM.len()] == SCHM {
        let uri = req.uri.clone();
        let substr = &uri[SCHM.len()..];
        match substr.find('/') {
            Some(pos) => {
                req.uri = substr[pos..].to_string();
            },
            None => {
                req.uri = "/".to_string();
            },
        }
    }
}

fn parse_header(head: String) -> Result<HttpRequest, &'static str> {
    let h:       Vec<&str> = head.splitn(2, "\r\n\r\n").collect();
    let heads:   Vec<&str> = h[0].split("\r\n").collect();
    let cmdline: Vec<&str> = heads[0].splitn(3, ' ').collect();
    
    let cmd = match cmdline[0] {
        "GET"     => HttpMethod::GET,
        "POST"    => HttpMethod::POST,
        "HEAD"    => HttpMethod::HEAD,
        "OPTIONS" => HttpMethod::OPTIONS,
        "PUT"     => HttpMethod::PUT,
        "DELETE"  => HttpMethod::DELETE,
        "TRACE"   => HttpMethod::TRACE,
        "PATCH"   => HttpMethod::PATCH,
        _         => return Err("Error: invalid HTTP method"),
    };
    let uri = cmdline[1];
    let hver = if cmdline.len() == 2 {
        HttpVersion::Http0_9
    } else {
        match cmdline[2] {
            "HTTP/1.0" => HttpVersion::Http1_0,
            "HTTP/1.1" => HttpVersion::Http1_1,
            _          => return Err("Error: unknown HTTP version specifier"),
        }
    };

    let mut map = HashMap::new();
    for i in 1..heads.len() {
        if heads[i].len() > 0 {
            let kv: Vec<&str> = heads[i].splitn(2, ": ").collect();
            map.insert(kv[0].to_string(), kv[1].to_string());
        }
    }

    let mut body = Vec::new();
    let _ = body.write_all(h[1].as_bytes());

    Ok(HttpRequest {
        method:  cmd,
        version: hver,
        uri:     uri.to_string(),
        headers: map,
        body:    body,
    })
}

fn read_data(sock: &mut TcpStream) -> Result<HttpRequest, std::io::Error> {
    //let to = Duration::new(30, 0);
    //let _ = sock.set_read_timeout(Some(to));
    
    let mut data: Vec<u8> = Vec::new();
    let mut buf = [0; 64*1024];
    loop {
        match sock.read(&mut buf) {
            Ok(readsize) => {
                if readsize == 0 { break; }
                //println!("{}", String::from_utf8_lossy(&buf[0..readsize]));
                data.write_all(&buf[0..readsize]).unwrap();
                if readsize < buf.len() { break; }
            },
            Err(e) => {
                return Err(e)
            },
        }
    }

    let b = parse_header(String::from_utf8(data.clone()).unwrap()).unwrap();
    Ok(b)
}

fn proxy(addr: String, port: u16) {
    let listener = TcpListener::bind(format!("{}:{}", addr, port)).unwrap();
    loop {
        match listener.accept() {
            Ok((mut sock, _)) => {
                thread::spawn(|| {
                    let mut req = read_data(&mut sock).unwrap();
                    camouflage_client(&mut req);
                    sendrecv_data(req, sock);
                });
            },
            Err(e) => {
                println!("Error: accept failed: {}", e);
                return
            },
        }
    }
}

fn main() {
    let opt = set_options(&mut env::args()).unwrap();
    proxy(opt.listen_addr, opt.listen_port)
}

