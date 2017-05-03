use std::net::{TcpListener};
use std::thread;
use std::io;
use std::io::{Read, Write};

fn start() -> io::Result<()> {
    let listener = TcpListener::bind("0.0.0.0:8080").unwrap();
    for mut stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let _: thread::JoinHandle<io::Result<()>> = thread::spawn(move || {
                    loop {
                        let mut b = [0; 1024];
                        let n = try!(stream.read(&mut b));
                        if n == 0 {
                            println!("{:?} closed.", try!(stream.peer_addr()));
                            return Ok(());
                        } else {
                            try!(stream.write(&b[0..n]));
                        }
                    }
                });
            }
            Err(e) => {}
        }
    }
    Ok(())
}

fn main()
{
    start();
}

