import * as Assets from '../assets';
import { ELOOP } from 'constants';
import * as Generated from '../deadfish_generated';
import { flatbuffers } from 'flatbuffers';
import WebSocketService from '../websocket';
import { FBUtil } from '../utils/fbutil';

const DEBUG_CAMERA = true;

const PIXELS2METERS = 0.01;
const METERS2PIXELS = 100;

export default class Gameplay extends Phaser.State {
    cursors = null;
    t: number;
    mobs = {};
    mySprite = null;

    public mouseHandler(e) {
        let worldX = this.input.activePointer.x;
        let worldY = this.input.activePointer.y;
        let builder = new flatbuffers.Builder(1);

        console.log(worldX, worldY, this.camera.x, this.camera.y);

        let serverX = (worldX+this.camera.x)*PIXELS2METERS;
        let serverY = (worldY+this.camera.y)*PIXELS2METERS;

        Generated.DeadFish.CommandMove.startCommandMove(builder);
        Generated.DeadFish.CommandMove.addTarget(builder,
            Generated.DeadFish.Vec2.createVec2(builder, serverX, serverY));
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

        this.game.input.activePointer.leftButton.onDown.add(this.mouseHandler, this);

        this.game.world.setBounds(-2000, -2000, 20000, 20000);

        this.cursors = this.game.input.keyboard.createCursorKeys();

        console.log(FBUtil.gameData);

        for (let bush of FBUtil.gameData.level.bushes) {
            let sprite = this.game.add.sprite(bush.x, bush.y, Assets.Images.ImagesBush.getName());
            sprite.anchor.x = 0.5;
            sprite.anchor.y = 0.5;
        }
        for (let stone of FBUtil.gameData.level.stones) {
            let sprite = this.game.add.sprite(stone.x, stone.y, Assets.Images.ImagesStone.getName());
            sprite.anchor.x = 0.5;
            sprite.anchor.y = 0.5;
        }

        WebSocketService.instance.getWebSocket().onmessage = (ev) => {
            (new Response(ev.data).arrayBuffer()).then(this.handleData.bind(this));
        };
    }

    public getSpriteBySpecies(species) {
        console.log("SPECIES", species);
        const adjustForSpecies = (l) => l.map((x) => (x+12*species));
        let sprite = this.game.add.sprite(150,150,Assets.Spritesheets.ImagesFish100100.getName());
        sprite.anchor.x = 0.5;
        sprite.anchor.y = 0.5;
    
        sprite.animations.add('idle', adjustForSpecies([0,1,2]));
        sprite.animations.add('moving', adjustForSpecies([3,4,5]));
        sprite.animations.add('attack', adjustForSpecies([6,7,8,9,10,11]));
    
        sprite.animations.play('idle', 3, true);
    
        return sprite;
    }

    public handleData(arrayBuffer) {
        let buffer = new Uint8Array(arrayBuffer);
        let dataState = FBUtil.ParseWorldState(buffer);
        if (dataState === null) {
            console.log("other type than world");
        }
        for (let mob in this.mobs) {
            this.mobs[mob].seen = false;
        }
        for (let i = 0; i < dataState.mobsLength(); i++) {
            let dataMob = dataState.mobs(i);
            let mob = this.mobs[dataMob.id()];
            if (mob === undefined) {
                console.log("making new sprite");
                mob = {
                    sprite: this.getSpriteBySpecies(dataMob.species())
                };
                this.mobs[dataMob.id()] = mob;
                if (dataMob.id() == FBUtil.gameData.initMeta.my_id) {
                    this.mySprite = mob.sprite;
                }
            }
            mob.sprite.position.x = dataMob.pos().x()*METERS2PIXELS;
            mob.sprite.position.y = dataMob.pos().y()*METERS2PIXELS;
            mob.sprite.angle = dataMob.angle()*180/Math.PI;
            mob.seen = true;
        }
        for (let mob in this.mobs) {
            if (!this.mobs[mob].seen) {
                this.mobs[mob].sprite.destroy();
                delete this.mobs[mob];
            }
        }
    }

    public update(): void {
        if (!this.mySprite)
            return;
        let offsetX = this.input.activePointer.x-(this.game.width/2);
        let offsetY = this.input.activePointer.y-(this.game.height/2);
        this.game.camera.x = (offsetX + this.mySprite.position.x)-(this.game.width/2);
        this.game.camera.y = (offsetY + this.mySprite.position.y)-(this.game.height/2);
    }

    public render(): void {
        // this.game.debug.cameraInfo(this.game.camera, 32, 32);
    }
}
