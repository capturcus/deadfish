import * as Assets from '../assets';
import { ELOOP } from 'constants';
import * as Generated from '../deadfish_generated';
import { flatbuffers } from 'flatbuffers';
import WebSocketService from '../websocket';
import { FBUtil } from '../utils/fbutil';

const DEBUG_CAMERA = true;

const PIXELS2METERS = 0.01;
const METERS2PIXELS = 100;

const CIRCLE_WIDTH = 15;

export default class Gameplay extends Phaser.State {
    t: number;
    mobs = {};
    mySprite: Phaser.Sprite = null;
    myGraphics: Phaser.Graphics = null;
    globalGraphics: Phaser.Graphics = null;
    running: boolean = false;
    bushGroup: Phaser.Group;

    public sendCommandRunning(running: boolean) {
        let builder = new flatbuffers.Builder(1);

        Generated.DeadFish.CommandRun.startCommandRun(builder);
        Generated.DeadFish.CommandRun.addRun(builder, running);
        let cmdRun = Generated.DeadFish.CommandRun.endCommandRun(builder);
        Generated.DeadFish.ClientMessage.startClientMessage(builder);
        Generated.DeadFish.ClientMessage.addEventType(builder, Generated.DeadFish.ClientMessageUnion.CommandRun);
        Generated.DeadFish.ClientMessage.addEvent(builder, cmdRun);
        let event = Generated.DeadFish.ClientMessage.endClientMessage(builder);
        builder.finish(event);

        let bytes = builder.asUint8Array();

        WebSocketService.instance.getWebSocket().send(bytes);
    }

    public mouseHandler(e) {
        let worldX = this.input.activePointer.x;
        let worldY = this.input.activePointer.y;
        let builder = new flatbuffers.Builder(1);

        console.log(worldX, worldY, this.camera.x, this.camera.y);

        let serverX = (worldX + this.camera.x) * PIXELS2METERS;
        let serverY = (worldY + this.camera.y) * PIXELS2METERS;

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

    public showText(text: string, color: string) {
        let spr = this.game.add.text(300, 100, text, {
            "fontSize": 20
        });
        spr.addColor(color, 0);
        this.game.add.tween(spr.cameraOffset).to({y: 0}, 1000, Phaser.Easing.Linear.None, true);
        this.game.add.tween(spr).to({alpha: 0}, 1500, Phaser.Easing.Linear.None, true);
        this.game.time.events.add(1500, () => {
            spr.destroy();
            spr = undefined;
        });
        spr.fixedToCamera = true;
    }

    public create(): void {
        this.game.camera.flash(0x000000, 1000);
        this.game.stage.backgroundColor = "#ffffff";

        this.game.input.activePointer.leftButton.onDown.add(this.mouseHandler, this);

        this.game.world.setBounds(-2000, -2000, 20000, 20000);

        this.game.canvas.oncontextmenu = function (e) { e.preventDefault(); }

        this.myGraphics = this.game.add.graphics(this.game.width/2-50, this.game.height/2+100);
        this.globalGraphics = this.game.add.graphics(0, 0);
        this.bushGroup = this.game.add.group();

        this.game.input.keyboard.onDownCallback = ((ev) => {
            if (ev.key === "q") {
                if (!this.running) {
                    this.running = true;
                    this.sendCommandRunning(this.running);
                }
            }
        });
        this.game.input.keyboard.onUpCallback = ((ev) => {
            if (ev.key === "q") {
                if (this.running) {
                    this.running = false;
                    this.sendCommandRunning(this.running);
                }
            }
        });

        console.log(FBUtil.gameData);

        for (let bush of FBUtil.gameData.level.bushes) {
            let sprite = this.game.add.sprite(bush.x, bush.y, Assets.Images.ImagesBush.getName());
            sprite.anchor.x = 0.5;
            sprite.anchor.y = 0.5;
            this.bushGroup.add(sprite);
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
        const adjustForSpecies = (l) => l.map((x) => (x + 12 * species));
        let sprite = this.game.add.sprite(150, 150, Assets.Spritesheets.ImagesFish100100.getName());
        sprite.anchor.x = 0.5;
        sprite.anchor.y = 0.5;

        sprite.animations.add('idle', adjustForSpecies([0, 1, 2]));
        sprite.animations.add('walking', adjustForSpecies([3, 4, 5]));
        sprite.animations.add('attacking', adjustForSpecies([6, 7, 8, 9, 10, 11]));

        sprite.animations.play('idle', 3, true);
        sprite.inputEnabled = true;
        sprite.hitArea = new PIXI.Rectangle(-25, -50, 50, 100);

        return sprite;
    }

    mobStateToAnimKey(s) {
        if (s === Generated.DeadFish.MobState.Idle) {
            return 'idle';
        }
        if (s === Generated.DeadFish.MobState.Walking) {
            return 'walking';
        }
        if (s === Generated.DeadFish.MobState.Attacking) {
            return 'attacking';
        }
    }

    sendKillCommand(id) {
        let buf = FBUtil.MakeKillCommand(id);
        WebSocketService.instance.getWebSocket().send(buf);
    }

    public handleData(arrayBuffer) {
        let buffer = new Uint8Array(arrayBuffer);
        let ev = FBUtil.ParseSimpleServerEvent(buffer);
        if (ev !== null) {
            if (ev === Generated.DeadFish.SimpleServerEventType.TooFarToKill) {
                this.showText("Mob too far to kill...", "#000");
                for (let k in this.mobs) {
                    let mob = this.mobs[k];
                    mob.targeted = false;
                }
            }
            if (ev === Generated.DeadFish.SimpleServerEventType.KilledCivilian) {
                this.showText("You killed a civilian. You monster.", "#000");
            }
            return;
        }
        let dataState = FBUtil.ParseWorldState(buffer);
        if (dataState === null) {
            console.log("other type than world");
            let killedPlayer = FBUtil.ParseKilledPlayer(buffer);
            if (killedPlayer !== null) {
                console.log("killed player", killedPlayer.playerName());
                this.showText("You killed "+killedPlayer.playerName()+"!", "#f00");
            }
            return;
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
                    sprite: this.getSpriteBySpecies(dataMob.species()),
                    state: dataMob.state(),
                    id: dataMob.id(),
                    targeted: false
                };
                this.mobs[dataMob.id()] = mob;
                if (dataMob.id() === FBUtil.gameData.initMeta.my_id) {
                    this.mySprite = mob.sprite;
                }
                mob.sprite.events.onInputDown.add((ev) => {
                    if (this.game.input.activePointer.rightButton.isDown
                        && mob.id !== FBUtil.gameData.initMeta.my_id) {
                        this.sendKillCommand(mob.id);
                        mob.targeted = true;
                    }
                });
            }
            mob.sprite.position.x = dataMob.pos().x() * METERS2PIXELS;
            mob.sprite.position.y = dataMob.pos().y() * METERS2PIXELS;
            mob.sprite.angle = dataMob.angle() * 180 / Math.PI;
            mob.seen = true;
            if (dataMob.state() != mob.state) {
                mob.state = dataMob.state();
                let animKey = this.mobStateToAnimKey(mob.state);
                console.log("change to", animKey);
                mob.sprite.animations.play(animKey, 3, true);
            }
        }
        for (let mob in this.mobs) {
            if (!this.mobs[mob].seen) {
                this.mobs[mob].sprite.destroy();
                delete this.mobs[mob];
            }
        }
        let indicators = [];
        for (let i = 0; i < dataState.indicatorsLength(); i++) {
            indicators.push({
                angle: dataState.indicators(i).angle(),
                force: 1-dataState.indicators(i).force(),
                visible: dataState.indicators(i).visible()
            });
        }
        this.drawIndicators(indicators);
    }

    public drawIndicators(indicators) {
        const FORCE_MULTIPLIER = Math.PI/2;
        indicators.sort((a,b) => a.force-b.force);
        this.myGraphics.clear();
        let outer = indicators.length;
        // let arc = (i) => {
        //     if (i.force !== 1) {
        //     this.myGraphics.arc(0, 0, 60+outer,
        //         i.angle+i.force*FORCE_MULTIPLIER,
        //         i.angle-i.force*FORCE_MULTIPLIER,
        //         true);
        //     } else {
        //         this.myGraphics.drawCircle(0, 0, (60+outer)*2);
        //     }
        // };
        for (let i of indicators) {
            if (i.visible)
                this.myGraphics.beginFill(Phaser.Color.BLUE, 0.5);
            else
                this.myGraphics.beginFill(Phaser.Color.GRAY, 0.2);
            this.myGraphics.drawCircle(0, 0, 60+outer*CIRCLE_WIDTH+CIRCLE_WIDTH*i.force);
            this.myGraphics.beginFill(Phaser.Color.WHITE, 1);
            this.myGraphics.drawCircle(0, 0, 60+outer*CIRCLE_WIDTH);
            outer--;
        }
    }

    public update(): void {
        if (!this.mySprite)
            return;
        this.myGraphics.position = this.mySprite.position;
        let offsetX = this.input.activePointer.x - (this.game.width / 2);
        let offsetY = this.input.activePointer.y - (this.game.height / 2);
        this.game.camera.x = (offsetX + this.mySprite.position.x) - (this.game.width / 2);
        this.game.camera.y = (offsetY + this.mySprite.position.y) - (this.game.height / 2);
        this.globalGraphics.clear();
        // this.globalGraphics.drawCircle(this.mySprite.position.x, this.mySprite.position.y, 100);
        for (let k in this.mobs) {
            let mob = this.mobs[k];
            if (mob.targeted) {
                this.globalGraphics.beginFill(Phaser.Color.RED, 0.2);
                this.globalGraphics.drawCircle(mob.sprite.x, mob.sprite.y, 100);
                continue;
            }
            if (mob.id !== FBUtil.gameData.initMeta.my_id &&
                Phaser.Math.distance(mob.sprite.x, mob.sprite.y, this.mySprite.x, this.mySprite.y) <= 100) {
                this.globalGraphics.beginFill(Phaser.Color.BLUE, 0.2);
                this.globalGraphics.drawCircle(mob.sprite.x, mob.sprite.y, 100);
            }
        }
        this.game.world.bringToTop(this.bushGroup);
    }

    public render(): void {
        // this.game.debug.cameraInfo(this.game.camera, 32, 32);
    }
}
