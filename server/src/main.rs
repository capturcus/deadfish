extern crate ws;

use ws::listen;

fn main() {
    println!("Hello, world!");

    // A WebSocket echo server
    listen("127.0.0.1:63987", |out| {
        move |msg| {
            println!("received: {}", msg);
            out.send(msg)
        }
    }).unwrap()
}
