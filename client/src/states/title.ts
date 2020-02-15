import * as Assets from '../assets';
import WebSocketService from '../websocket';
import * as fbutil from '../utils/fbutil';
import * as Generated from '../deadfish_generated';

function makePlayerId(length) {
    var result = '';
    var characters = 'abcdefghijklmnopqrstuvwxyz';
    var charactersLength = characters.length;
    for (var i = 0; i < length; i++) {
        result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
}

export default class Title extends Phaser.State {

    public create(): void {
        let el = document.getElementById("htmlStuff");
        el.setAttribute("style", "position:fixed; left: 50%; transform: translateX(-50%); top: 80vh; display: flex; flex-direction: column;");
        this.game.stage.backgroundColor = "#ffffff";

        let logo = this.game.add.sprite(this.game.width / 2, 0, Assets.Images.ImagesLogo.getName());
        logo.x -= logo.width / 2;

        let nameText = document.getElementById("myName");
        nameText.setAttribute("value", makePlayerId(5));

        // FIXME: Change the ID of the button
        let butt = document.getElementById("myButton");
        butt.onclick = (ev) => {
            let nameText: any = document.getElementById("myName");
            let serverText: any = document.getElementById("myText");
            fbutil.FBUtil.gameData.myName = nameText.value;
            if (fbutil.FBUtil.gameData.myName === "") {
                alert("please name yourself");
                return;
            }
            let data = fbutil.FBUtil.MakeJoinRequest(fbutil.FBUtil.gameData.myName);
            WebSocketService.instance.init("ws://" + serverText.value);
            WebSocketService.instance.getWebSocket().onerror = (e) => {
                alert("failed to connect to server");
            }
            WebSocketService.instance.getWebSocket().onclose = (e) => {
                // alert("lost connection to server");
                // location.reload();
            }
            WebSocketService.instance.getWebSocket().onopen = (ev) => {
                console.log("connected");
                WebSocketService.instance.getWebSocket().onmessage = (data) => this.onWebSocket(data);
                WebSocketService.instance.getWebSocket().send(data);
            };
        }
    }

    async onWebSocket(data: MessageEvent) {
        console.log("received", data);
        let arrayBuffer = await new Response(data.data).arrayBuffer();
        let buffer = new Uint8Array(arrayBuffer);
        // parse InitMetadata and save it in FBUtil, fail silently if it's not InitMetadata
        fbutil.FBUtil.ParseInitMetadata(buffer);
        document.getElementById("htmlStuff").setAttribute("style", "display: none;");
        console.log("going into lobby");
        this.game.state.start('lobby');
    }
}