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
const ROUND_LENGTH = 10 * 60;
const ANIM_FPS = 20;

function pad(num:number, size:number): string {
    let s = num+"";
    while (s.length < size) s = "0" + s;
    return s;
}

export default class Gameplay extends Phaser.State {
    t: number;
    mobs = {};
    mySprite: Phaser.Sprite = null;
    myGraphics: Phaser.Graphics = null;
    globalGraphics: Phaser.Graphics = null;
    running: boolean = false;
    bushGroup: Phaser.Group;
    textGroup: Phaser.Group;
    secondsLeft: number = ROUND_LENGTH;
    timerText: Phaser.Text;

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
        let spr = this.add.text(0, 300, text, {
            "fontSize": 60
        });
        spr.position.x = this.game.width / 2 - spr.width / 2;
        this.textGroup.add(spr);
        spr.addColor(color, 0);
        this.game.add.tween(spr.cameraOffset).to({ y: 0 }, 1000, Phaser.Easing.Linear.None, true);
        this.game.add.tween(spr).to({ alpha: 0 }, 1500, Phaser.Easing.Linear.None, true);
        this.game.time.events.add(1500, () => {
            spr.destroy();
            spr = undefined;
        });
        spr.fixedToCamera = true;
    }

    public updateHighscores() {
        let highscores = document.getElementById("highscorestable");
        let html = "";
        for (let p of FBUtil.gameData.highscores) {
            html += "<tr><td>" + p.name + "</td><td>" + p.points + "</td></tr>";
        }
        highscores.innerHTML = html;
    }

    public renderHighscores() {
        let highscores = document.getElementById("highscorestable");
        highscores.setAttribute("style", "display: block;");
        this.updateHighscores();
    }

    public secondsToText(): string {
        let secs = this.secondsLeft % 60;
        let mins = Math.floor(this.secondsLeft / 60);
        return pad(mins, 2) + ":" + pad(secs, 2);
    }

    advanceTimer() {
        if(!FBUtil.gameData.gameRunning)
            return;
        this.secondsLeft--;
        this.timerText.text = this.secondsToText();
    }

    public create(): void {
        this.game.camera.flash(0x000000, 1000);
        this.game.stage.backgroundColor = "#ffffff";

        this.game.input.activePointer.leftButton.onDown.add(this.mouseHandler, this);

        this.game.world.setBounds(-2000, -2000, 20000, 20000);

        this.game.canvas.oncontextmenu = function (e) { e.preventDefault(); }

        this.myGraphics = this.game.add.graphics(this.game.width / 2 - 50, this.game.height / 2 + 100);
        this.globalGraphics = this.game.add.graphics(0, 0);
        this.bushGroup = this.game.add.group();
        this.textGroup = this.game.add.group();
        this.timerText = this.game.add.text(0, 20, this.secondsToText(), {
            "fontSize": 60
        });
        this.timerText.position.x = this.game.width - (this.timerText.width / 2) - 100;
        this.timerText.fixedToCamera = true;
        this.textGroup.add(this.timerText);

        setInterval(this.advanceTimer.bind(this), 1000);

        for (let player of FBUtil.gameData.initMeta.players) {
            FBUtil.gameData.highscores.push({ name: player.name, points: 0 });
        }

        this.game.input.keyboard.onDownCallback = ((ev) => {
            if (ev.key === "q") {
                if (!this.running) {
                    this.running = true;
                    this.sendCommandRunning(this.running);
                }
            }
            if (ev.key === "a") {
                this.renderHighscores();
            }
        });
        this.game.input.keyboard.onUpCallback = ((ev) => {
            if (ev.key === "q") {
                if (this.running) {
                    this.running = false;
                    this.sendCommandRunning(this.running);
                }
            }
            if (ev.key === "a" && FBUtil.gameData.gameRunning) {
                let highscores = document.getElementById("highscorestable");
                highscores.setAttribute("style", "display: none;");
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

    public spriteNums(max: number, offset: number, species: number) {
        let ret = [];
        for (let i = 0; i < max; i++) {
            ret.push(i+offset+species*80);
        }
        return ret;
    }
    
    public getSpriteBySpecies(species) {
        let sprite = this.game.add.sprite(150, 150, Assets.Spritesheets.ImagesFish100100.getName());
        sprite.anchor.x = 0.5;
        sprite.anchor.y = 0.5;

        sprite.animations.add('idle', this.spriteNums(20, 0, species));
        sprite.animations.add('walking', this.spriteNums(20, 20, species));
        sprite.animations.add('attacking', this.spriteNums(40, 40, species));

        sprite.animations.play('idle', ANIM_FPS, true);
        sprite.inputEnabled = true;
        sprite.hitArea = new Phaser.Circle(0, 0, 100);

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
            if (ev === Generated.DeadFish.SimpleServerEventType.GameEnded) {
                FBUtil.gameData.gameRunning = false;
                let spr = this.game.add.text(0, 100, "GAME OVER", {
                    "fontSize": 80,
                    stroke: "#000",
                    strokeThickness: 10
                });
                spr.position.x = this.game.width / 2 - spr.width / 2;
                spr.addColor("#ff0000", 0);
                spr.fixedToCamera = true;
                this.renderHighscores();
            }
            return;
        }
        let dataState = FBUtil.ParseWorldState(buffer);
        if (dataState === null) {
            console.log("other type than world");
            let deathReport = FBUtil.ParseDeathReport(buffer);
            if (deathReport !== null) {
                if (deathReport.killer() == FBUtil.gameData.myName)
                    this.showText("You killed " + deathReport.killed(), "#ff0000");
                else if (deathReport.killed() == FBUtil.gameData.myName)
                    this.showText("You were killed by " + deathReport.killer(), "#ff0000");
                else
                    this.showText(deathReport.killer() + " killed " + deathReport.killer(), "#000");
                return;
            }
            let highscores = FBUtil.ParseHighscoreUpdate(buffer);
            if (highscores !== null) {
                FBUtil.gameData.highscores = [];
                for (let i = 0; i < highscores.playersLength(); i++) {
                    let player = highscores.players(i);
                    FBUtil.gameData.highscores.push({ "name": player.playerName(), "points": player.playerPoints() });
                }
                FBUtil.gameData.highscores.sort((a, b) => b.points - a.points);

                this.updateHighscores();
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
                mob.sprite.animations.play(animKey, ANIM_FPS, true);
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
                force: 1 - dataState.indicators(i).force(),
                visible: dataState.indicators(i).visible()
            });
        }
        this.drawIndicators(indicators);
    }

    public drawIndicators(indicators) {
        const FORCE_MULTIPLIER = Math.PI / 2;
        indicators.sort((a, b) => a.force - b.force);
        this.myGraphics.clear();
        let outer = indicators.length;
        for (let i of indicators) {
            if (i.visible)
                this.myGraphics.beginFill(Phaser.Color.BLUE, 0.5);
            else
                this.myGraphics.beginFill(Phaser.Color.GRAY, 0.2);
            this.myGraphics.drawCircle(0, 0, 60 + outer * CIRCLE_WIDTH + CIRCLE_WIDTH * i.force);
            this.myGraphics.beginFill(Phaser.Color.WHITE, 1);
            this.myGraphics.drawCircle(0, 0, 60 + outer * CIRCLE_WIDTH);
            outer--;
        }
    }

    public update(): void {
        if (!this.mySprite || !FBUtil.gameData.gameRunning)
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
                mob.sprite.input.pointerOver()) {
                this.globalGraphics.beginFill(Phaser.Color.GRAY, 0.2);
                this.globalGraphics.drawCircle(mob.sprite.x, mob.sprite.y, 100);
            }
            if (mob.id !== FBUtil.gameData.initMeta.my_id &&
                Phaser.Math.distance(mob.sprite.x, mob.sprite.y, this.mySprite.x, this.mySprite.y) <= 100) {
                this.globalGraphics.beginFill(Phaser.Color.BLUE, 0.2);
                this.globalGraphics.drawCircle(mob.sprite.x, mob.sprite.y, 100);
            }
        }
        this.game.world.bringToTop(this.bushGroup);
        this.game.world.bringToTop(this.textGroup);
    }

    public render(): void {
        // this.game.debug.cameraInfo(this.game.camera, 32, 32);
    }
}
