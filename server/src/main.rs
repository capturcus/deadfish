extern crate flatbuffers;
extern crate nalgebra as na;

use std::{thread, time};
use std::f64::consts::PI;
use std::ops;

use websocket::server::sync::Server;
use websocket::Message;
use websocket::websocket_base;
use na::{Vector2, Rotation2};

mod deadfish_generated;
use deadfish_generated::dead_fish;

type ClientSocket = websocket::client::sync::Client<std::net::TcpStream>;
type Vec2 = Vector2<f32>;

const CONST_SLEEP: u64 = 40;

struct Player {
    id: u16,
    position: Vec2,
    angle: f32,
    socket: ClientSocket,
    target_position: Vec2,
}

impl PartialEq for Player {
    fn eq(&self, rhs: &Player) -> bool {
        self.id == rhs.id
    }
}

impl Player {
    pub fn send_data(&mut self) -> std::result::Result<(), websocket_base::result::WebSocketError> {
        let mut builder = flatbuffers::FlatBufferBuilder::new_with_capacity(1);
        let mob = dead_fish::Mob::create(&mut builder, &dead_fish::MobArgs{
            id: 0,
            pos: Some(&fb_vec2(&self.position)),
            angle: self.angle,
            state: dead_fish::MobState::Walking,
            species: 0,
        });
        let mobs = builder.create_vector(&[mob]);
        let data_state = dead_fish::DataState::create(&mut builder, &dead_fish::DataStateArgs{
            mobs: Some(mobs)
        });
        builder.finish(data_state, None);
        self.socket.send_message(&Message::binary(builder.finished_data()))
    }

    pub fn update(&mut self) {
        let to_target = self.target_position - self.position;
        if to_target.norm() <= 3.{
            return;
        }
        let to_target = to_target.normalize();
        let heading = Rotation2::rotation_between(&Vector2::x(), &to_target);
        let angle = heading.angle().to_degrees();
        self.position += to_target*3.;
        self.angle = angle+90.;
    }
}

fn fb_vec2(v: &Vec2) -> dead_fish::Vec2 {
    dead_fish::Vec2::new(v.x, v.y)
}


fn main() {
    let mut server = Server::bind("127.0.0.1:63987").unwrap();
    let mut current_player_id: u16 = 0;

    // Set the server to non-blocking.
    server.set_nonblocking(true).unwrap();

    let mut players = Vec::new();

    loop {
        match server.accept() {
            Ok(wsupgrade) => {
                // Do something with the established TcpStream.
                println!("CONN!!!");
                let client = wsupgrade.accept().unwrap();
                client.set_nonblocking(true).unwrap();
                players.push(Player{
                    id: current_player_id,
                    position: Vec2::new(0., 0.),
                    angle: 0.,
                    socket: client,
                    target_position: Vec2::new(100., 100.),
                });
                current_player_id += 1;
            }
            _ => {
                // Nobody tried to connect, move on.
            }
        };
        // println!("no siema");
        thread::sleep(time::Duration::from_millis(CONST_SLEEP));
        let mut players_to_delete = Vec::new();
        for i in 0..players.len() {
            let mut player = &mut players[i];
            match player.socket.recv_dataframe() {
                Ok(data) => {
                    if data.data.len() < 4 {
                        // refresh ?
                        continue
                    }
                    let cmd_move = flatbuffers::get_root::<dead_fish::CommandMove>(&data.data);
                    println!("cmd_move {} {}", cmd_move.target().unwrap().x(), cmd_move.target().unwrap().y());
                    player.target_position = Vec2::new(cmd_move.target().unwrap().x(), cmd_move.target().unwrap().y());
                }
                _ => {
                }
            }
            player.update();

            match player.send_data() {
                Ok(_) => {}
                _ => {
                    players_to_delete.push(i);
                }
            }
        }
        for i in players_to_delete {
            println!("removing player {}", i);
            players.swap_remove(i);
        }
    }
}
