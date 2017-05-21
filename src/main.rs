use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
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
    Http09,
    Http10,
    Http11,
}

impl fmt::Display for HttpVersion {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", match self {
            &HttpVersion::Http09 => "",
            &HttpVersion::Http10 => "HTTP/1.0",
            &HttpVersion::Http11 => "HTTP/1.1",
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

impl HttpRequest {
    fn contentlength(&mut self) -> Option<u32> {
        match self.has_key("Content-Length".to_string()) {
            Some(s) => Some(s.parse::<u32>().unwrap()),
            None    => None,
        }
    }
    fn keepalive(&mut self) -> Option<()> {
        match self.has_key("Proxy-Connection".to_string()) {
            Some(_) => Some(()),
            None    => None,
        }
    }
    fn has_key(&mut self, key: String) -> Option<String> {
        match self.headers.entry(key) {
            Occupied(e) => Some(e.get().clone()),
            Vacant(_)   => None,
        }
    }
    fn insert_header(&mut self, key: String, val: String) {
        self.has_key(key.clone()).unwrap_or_else(move || {
            self.headers.insert(key, val);
            "".to_string()
        });
    }
    fn remove_header(&mut self, key: String) {
        self.has_key(key.clone()).and_then(|| self.headers.remove(key));
    }
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
    verbose:     i32,
}

fn print_usage(args:&mut env::Args) {
    println!("Usage: {}", args.nth(0).unwrap());
}

fn set_options(args:&mut env::Args) -> Result<RToption, String> {
    let o = RToption {
        listen_addr: "0.0.0.0".to_string(),
        listen_port: 8080,
        verbose:     0,
    };
    Ok(o)
}

fn find_vec_in_vec(vec: &Vec<u8>, sdata: &[u8], pos: usize) -> (usize, bool) {
    let len1 = vec.len();
    let len2 = sdata.len();
    if len1 < len2 {
        return (0, false)
    }
    for i in pos..len1-len2+1 {
        if &vec[i..i+len2] == sdata {
            return (i, true)
        }
    }
    (len1, false)
}

#[test]
fn test_find_vec_in_vec() {
    {
        let vec = vec![0, 1, 2, 3, 4];
        let sd = [0, 1, 2, 3, 4];
        assert_eq!(find_vec_in_vec(&vec, &sd, 0), (0, true));
    }
    {
        let vec = vec![0, 1, 2, 3, 4];
        let sd = [0, 1];
        assert_eq!(find_vec_in_vec(&vec, &sd, 0), (0, true));
    }
    {
        let vec = vec![0, 1, 2, 3, 4];
        let sd = [1, 2];
        assert_eq!(find_vec_in_vec(&vec, &sd, 0), (1, true));
    }
    {
        let vec = vec![0, 1, 2, 3, 4];
        let sd = [3, 4];
        assert_eq!(find_vec_in_vec(&vec, &sd, 0), (3, true));
    }
    {
        let vec = vec![0, 1, 2, 3, 4];
        let sd = [2, 3, 4];
        assert_eq!(find_vec_in_vec(&vec, &sd, 0), (2, true));
    }
    
    {
        let vec = vec![0, 1, 2, 3, 4];
        let sd = [7, 8];
        assert_eq!(find_vec_in_vec(&vec, &sd, 0), (5, false));
    }

    {
        let vec = vec![0, 1, 2, 3, 4];
        let sd = [3, 4];
        assert_eq!(find_vec_in_vec(&vec, &sd, 3), (3, true));
    }
    {
        let vec = vec![0, 1, 2, 3, 4];
        let sd = [3, 4];
        assert_eq!(find_vec_in_vec(&vec, &sd, 4), (5, false));
    }

    {
        let vec = vec![0, 1];
        let sd = [0, 1, 2];
        assert_eq!(find_vec_in_vec(&vec, &sd, 0), (0, false));
    }

    {
        let vec = vec![0, 1, 2, 3];
        let sd = [0, 1, 2];
        assert_eq!(find_vec_in_vec(&vec, &sd, 10), (4, false));
    }
}

/*
fn sendrecv_data(mut req: HttpRequest, mut sock: TcpStream) -> Result<(), String> {
    let host = match req.headers.entry("Host".to_string()) {
        Occupied(e) => {
            e.get().clone()
        },
        Vacant(_) => {
            return Err("Error: no Host header founds in HTTP request".to_string())
        },
    };

    let host = match host.find(':') {
        Some(_) => host,
        None    => format!("{}:80", host),
    };
    let mut s = match TcpStream::connect(host) {
        Ok(s)  => s,
        Err(e) => return Err(e.to_string()),
    };

    let _ = s.write_all(format!("{}", req).as_bytes());

    loop {
        let mut buf = [0; 64*1024];
        let readsize = s.read(&mut buf).unwrap();
        if readsize == 0 {
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

fn parse_header(head: String) -> Result<HttpRequest, String> {
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
        _         => {
            return Err(format!("Error: invalid HTTP method({})", cmdline[0]))
        }
    };
    let uri = cmdline[1];
    let hver = if cmdline.len() == 2 {
        HttpVersion::Http0_9
    } else {
        match cmdline[2] {
            "HTTP/1.0" => HttpVersion::Http1_0,
            "HTTP/1.1" => HttpVersion::Http1_1,
            _          => {
                return Err(format!("Error: unknown HTTP version specifier({})", cmdline[2]))
            }
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

fn recv_http_request(sock: &mut TcpStream) -> Result<HttpRequest, std::io::Error> {
    let to = Duration::new(30, 0);
    let _ = sock.set_read_timeout(Some(to));
    
    let mut data: Vec<u8> = Vec::new();
    let mut buf = [0; 64*1024];
    loop {
        match sock.read(&mut buf) {
            Ok(readsize) => {
                if readsize == 0 {
                    break;
                }
                //println!("{}", String::from_utf8_lossy(&buf[0..readsize]));
                data.write_all(&buf[0..readsize]).unwrap();
                if data.len() > 4 &&
                    data[data.len()-4..data.len()] == [13, 10, 13, 10] {
                    break;
                }
            },
            Err(e) => {
                return Err(e)
            },
        }
    }

    let req_str = String::from_utf8_lossy(data.as_slice()).into_owned();
    let b = parse_header(req_str.to_string()).unwrap();
    //println!("{}", b);
    Ok(b)
}

fn proxy(addr: String, port: u16) {
    let listener = TcpListener::bind(format!("{}:{}", addr, port)).unwrap();
    loop {
        match listener.accept() {
            Ok((mut sock, _)) => {
                thread::spawn(move || {
                    let mut req = recv_http_request(&mut sock).unwrap();
                    camouflage_client(&mut req);
                    sendrecv_data(req, sock);
                });
            },
            Err(e) => {
                println!("Error: accept failed: {}", e);
            },
        }
    }
}
*/

fn http_request_parse<F>(s:& F) -> Result<HttpRequest, String>
where F: FnOnce(usize) -> Result<Vec<u8>, String> {
    
    Err("".to_string())
}

fn proxy(addr: String, port: u16) -> Result<(), String> {
    let listener = TcpListener::bind(format!("{}:{}", addr, port)).unwrap();
    loop {
        match listener.accept() {
            Ok((mut sock, _)) => {
                let to = Duration::new(30, 0);
                let _ = sock.set_read_timeout(Some(to));
                thread::spawn(move || {
                    let mut sock = sock;
                    let tcp_reader = |req: usize| -> Result<Vec<u8>, String> {
                        let mut buf = [0; 32*1024];
                        let mut data: Vec<u8> = Vec::new();
                        let mut amount:usize = 0;
                        loop {
                            match sock.read(&mut buf[0..req-amount]) {
                                Ok(readsize) => {
                                    amount += readsize;
                                    data.write_all(&buf[0..readsize]).unwrap();
                                },
                                Err(e) => {
                                    return Err(format!("{}", e))
                                },
                            }
                            if amount == req {
                                break;
                            }
                        }
                        Ok(data)
                    };

                    loop {
                        let req = http_request_parse(&tcp_reader);
                    }
                });
            },
            Err(e) => {
                return Err(format!("{}", e))
            },
        }
    }
}

fn main() {
    let opt = set_options(&mut env::args()).unwrap();
    proxy(opt.listen_addr, opt.listen_port);
}

