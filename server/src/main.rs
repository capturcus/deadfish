use websocket::server::sync::Server;
use websocket::Message;
use std::{thread, time};

fn main() {
    let mut server = Server::bind("127.0.0.1:63987").unwrap();

    // Set the server to non-blocking.
    server.set_nonblocking(true).unwrap();

    let hundred_ms = time::Duration::from_millis(100);

    let mut clients = Vec::new();

    loop {
        match server.accept() {
            Ok(wsupgrade) => {
                // Do something with the established TcpStream.
                println!("CONN!!!");
                let client = wsupgrade.accept().unwrap();
                clients.push(client);
            }
            _ => {
                // Nobody tried to connect, move on.
            }
        };
        // println!("no siema");
        thread::sleep(hundred_ms);
        for client in &mut clients {
            let data = client.recv_dataframe().unwrap();
            println!("payload len {}", data.data.len());
            client.send_message(&Message::binary(data.data)).unwrap();
        }
    }
}
