import * as Assets from '../assets';
import WebSocketService from '../websocket';
import * as fbutil from '../utils/fbutil';
import * as Generated from '../deadfish_generated';

export default class Gameplay extends Phaser.State {

    public create(): void {
        // WebSocketService.instance.init("ws://127.0.0.1:63987");
        this.game.stage.backgroundColor = "#ffffff";

        this.game.add.sprite(this.game.width/2-580/2, 0, Assets.Images.ImagesLogo.getName());

        let butt = document.getElementById("myButton");
        butt.onclick = (ev) => {
            let nameText: any = document.getElementById("myName");
            let serverText: any = document.getElementById("myText");
            if (nameText.value === "") {
                alert("please name yourself");
                return;
            }
            let data = fbutil.FBUtil.MakeJoinRequest(nameText.value);
            WebSocketService.instance.init("ws://"+serverText.value);
            WebSocketService.instance.getWebSocket().onerror = (e) => {
                alert("failed to connect to server");
            }
            WebSocketService.instance.getWebSocket().onopen = (ev) => {
                console.log("connected");
                WebSocketService.instance.getWebSocket().send(data);
                WebSocketService.instance.getWebSocket().onmessage = (data) => {
                    console.log("received", data);
                    (new Response(data.data).arrayBuffer()).then((arrayBuffer) => {
                        let buffer = new Uint8Array(arrayBuffer);
                        fbutil.FBUtil.ParseInitMetadata(buffer);
                    });
                }
            };
        }
    }
}