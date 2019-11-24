import * as Assets from '../assets';
import { ELOOP } from 'constants';
import * as Generated from '../deadfish_generated';
import { flatbuffers } from 'flatbuffers';
// import websocket = require('websocket');

const DEBUG_CAMERA = true;

export default class Title extends Phaser.State {
    cursors = null;
    webSocket: WebSocket = null;
    t: number;

    public mouseHandler(e) {
        let worldX = e.event.x+this.game.camera.x;
        let worldY = e.event.y+this.game.camera.y;
        let builder = new flatbuffers.Builder(1);

        Generated.DeadFish.CommandMove.startCommandMove(builder);
        Generated.DeadFish.CommandMove.addTarget(builder,
            Generated.DeadFish.Vec2.createVec2(builder, worldX, worldY));
        let firstCmdMove = Generated.DeadFish.CommandMove.endCommandMove(builder);
        builder.finish(firstCmdMove);

        let bytes = builder.asUint8Array();

        this.webSocket.send(bytes);
        this.t = performance.now();
    }

    public create(): void {
        this.game.camera.flash(0x000000, 1000);
        this.game.stage.backgroundColor = "#ffffff";

        let sprite = this.game.add.sprite(150,150,Assets.Spritesheets.ImagesFish100100.getName());
        sprite.anchor.x = 0.5;
        sprite.anchor.y = 0.5;

        sprite.animations.add('idle', [0,1,2]);
        sprite.animations.add('moving', [3,4,5]);
        sprite.animations.add('attack', [6,7,8,9,10,11]);

        sprite.animations.play('idle', 3, true);

        this.game.input.activePointer.leftButton.onDown.add(this.mouseHandler, this);

        this.game.world.setBounds(0, 0, 20000, 20000);

        this.cursors = this.game.input.keyboard.createCursorKeys();

        this.webSocket = new WebSocket("ws://127.0.0.1:63987");
        this.webSocket.addEventListener("open", (ev) => {
            console.log("open", ev);
        });
        this.webSocket.addEventListener("message", (ev) => {
            (new Response(ev.data).arrayBuffer()).then((arrayBuffer) => {
                let newBuffer = new flatbuffers.ByteBuffer(new Uint8Array(arrayBuffer));
                console.log(ev.data, newBuffer);
                let cmdMove = Generated.DeadFish.CommandMove.getRootAsCommandMove(newBuffer);
                console.log(cmdMove.target().x(), cmdMove.target().y());
                console.log("lag:", (performance.now()-this.t));
            });
        });
    }

    public update(): void {
        if (DEBUG_CAMERA) {
            if (this.cursors.up.isDown) {
                this.game.camera.y -= 4;
            }
            else if (this.cursors.down.isDown)
            {
                this.game.camera.y += 4;
            }
        
            if (this.cursors.left.isDown)
            {
                this.game.camera.x -= 4;
            }
            else if (this.cursors.right.isDown)
            {
                this.game.camera.x += 4;
            }
        }
    }

    public render(): void {
        this.game.debug.cameraInfo(this.game.camera, 32, 32);
    }
}
