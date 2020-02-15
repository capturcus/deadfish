import 'phaser';
import WebSocketService from '../websocket';
import * as fbutil from '../utils/fbutil';
import { DeadFish } from '../deadfish_generated';

export default class Lobby extends Phaser.State {
    text;

    private setText() {
        this.text.text = "welcome to the ocean\n\nfish:\n";
        for (let player of fbutil.FBUtil.gameData.initMeta.players) {
            this.text.text += player.name + "\n";
        }
    }

    sendReady() {
        let buf = fbutil.FBUtil.MakePlayerReady();
        WebSocketService.instance.getWebSocket().send(buf);
        let btn: any = document.getElementById("readyButton");
        btn.disabled = true;
    }

    public create(): void {
        console.log("lobby");
        this.text = this.game.add.text(300, 100, "", {
            "fontSize": 20
        });
        this.setText();
        document.getElementById("lobbyStuff").setAttribute("style", "height: 100vh;position: fixed;left: 50%;transform: translateX(-50%);top: 80vh;");
        document.getElementById("readyButton").onclick = ((ev) => {
            this.sendReady();
        });
        WebSocketService.instance.getWebSocket().onopen = (ev) => {};
        WebSocketService.instance.getWebSocket().onmessage = (data) => this.onWebSocket(data);
    }

    async onWebSocket(data: MessageEvent) {
        console.log("onWebSocket!");
        let arrayBuffer = await new Response(data.data).arrayBuffer();
        let buffer = new Uint8Array(arrayBuffer);
        console.log(buffer);
        if(fbutil.FBUtil.ParseInitMetadata(buffer)) {
            console.log("was not init");
            if(fbutil.FBUtil.ParseLevel(buffer)) {
                document.getElementById("lobbyStuff").setAttribute("style", "display: none;");
                this.game.state.start('gameplay', true);
            }
        }
        this.setText();
    }
}