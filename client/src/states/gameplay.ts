import * as Assets from '../assets';
import { ELOOP } from 'constants';
import * as Generated from '../deadfish_generated';
import { flatbuffers } from 'flatbuffers';
import WebSocketService from '../websocket';

const DEBUG_CAMERA = true;

export default class Gameplay extends Phaser.State {
    cursors = null;
    t: number;

    public mouseHandler(e) {
        let worldX = this.input.activePointer.x;
        let worldY = this.input.activePointer.y;
        let builder = new flatbuffers.Builder(1);

        console.log(worldX, worldY);

        Generated.DeadFish.CommandMove.startCommandMove(builder);
        Generated.DeadFish.CommandMove.addTarget(builder,
            Generated.DeadFish.Vec2.createVec2(builder, worldX, worldY));
        let firstCmdMove = Generated.DeadFish.CommandMove.endCommandMove(builder);
        Generated.DeadFish.ClientMessage.startClientMessage(builder);
        Generated.DeadFish.ClientMessage.addEventType(builder, Generated.DeadFish.ClientMessageUnion.CommandMove);
        Generated.DeadFish.ClientMessage.addEvent(builder, firstCmdMove);
        let event = Generated.DeadFish.ClientMessage.endClientMessage(builder);
        builder.finish(event);

        let bytes = builder.asUint8Array();

        WebSocketService.instance.getWebSocket().send(bytes);
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

        // WebSocketService.instance.getWebSocket().addEventListener("message", (ev) => {
        //     (new Response(ev.data).arrayBuffer()).then((arrayBuffer) => {
        //         let newBuffer = new flatbuffers.ByteBuffer(new Uint8Array(arrayBuffer));
        //         let dataState = Generated.DeadFish.DataState.getRootAsDataState(newBuffer);
        //         let mob = dataState.mobs(0);
        //         // console.log(mob.angle(), mob.id(), mob.pos().x(), mob.pos().y(), mob.species(), mob.species());
        //         sprite.position.x = mob.pos().x();
        //         sprite.position.y = mob.pos().y();
        //         sprite.angle = mob.angle();
        //     });
        // });
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
