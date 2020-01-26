import 'phaser';
import WebSocketService from '../websocket';
import * as fbutil from '../utils/fbutil';
import { DeadFish } from '../deadfish_generated';

export default class Title extends Phaser.State {
    text;

    private setText() {
        this.text.text = "welcome to the ocean\n\nfish:\n";
        for (let player of fbutil.FBUtil.gameData.initMeta.players) {
            this.text.text += player.name + "\n";
        }
        // this.text.text += "\n\nwaiting for " + (3-fbutil.FBUtil.gameData.initMeta.players.length) + " more fish"
    }

    public create(): void {
        console.log("lobby");
        this.text = this.game.add.text(300, 100, "", {
            "fontSize": 20
        });
        this.setText();
        let that = this;
        WebSocketService.instance.getWebSocket().onopen = (ev) => {};
        WebSocketService.instance.getWebSocket().onmessage = (data) => {
            (new Response(data.data).arrayBuffer()).then((arrayBuffer) => {
                let buffer = new Uint8Array(arrayBuffer);
                if(fbutil.FBUtil.ParseInitMetadata(buffer)) {
                    console.log("was not init");
                    if(fbutil.FBUtil.ParseLevel(buffer)) {
                        that.game.state.start('gameplay', true);
                    }
                }
                this.setText();
            });
        }
    }
}