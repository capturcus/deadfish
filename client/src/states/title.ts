import * as Assets from '../assets';
import WebSocketService from '../websocket';
import * as fbutil from '../utils/fbutil';
import * as Generated from '../deadfish_generated';

function makeid(length) {
    var result           = '';
    var characters       = 'abcdefghijklmnopqrstuvwxyz';
    var charactersLength = characters.length;
    for ( var i = 0; i < length; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
 }

export default class Title extends Phaser.State {

    public create(): void {
        // WebSocketService.instance.init("ws://127.0.0.1:63987");
        let el = document.getElementById("htmlStuff");
        el.setAttribute("style", "position:fixed; left: 50%; transform: translateX(-50%); top: 80vh; display: flex; flex-direction: column;");
        this.game.stage.backgroundColor = "#ffffff";

        this.game.add.sprite(this.game.width/2-580/2, 0, Assets.Images.ImagesLogo.getName());

        let nameText = document.getElementById("myName");
        nameText.setAttribute("value", makeid(5));

        let butt = document.getElementById("myButton");
        let that = this;
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
            WebSocketService.instance.getWebSocket().onclose = (e) => {
                // alert("lost connection to server");
                location.reload();
            }
            WebSocketService.instance.getWebSocket().onopen = (ev) => {
                console.log("connected");
                WebSocketService.instance.getWebSocket().send(data);
                WebSocketService.instance.getWebSocket().onmessage = (data) => {
                    console.log("received", data);
                    (new Response(data.data).arrayBuffer()).then((arrayBuffer) => {
                        let buffer = new Uint8Array(arrayBuffer);
                        if(fbutil.FBUtil.ParseInitMetadata(buffer)) {
                            if(fbutil.FBUtil.ParseSimpleServerEvent(buffer) === Generated.DeadFish.SimpleServerEventType.GameStart) {
                                console.log("starting gameplay");
                                document.getElementById("htmlStuff").setAttribute("style", "display: none;");
                                that.game.state.start('gameplay', true);
                                return;
                            }
                        }
                        document.getElementById("htmlStuff").setAttribute("style", "display: none;");
                        console.log("going into lobby");
                        that.game.state.start('lobby');
                    });
                }
            };
        }
    }
}