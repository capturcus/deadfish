import * as Assets from '../assets';
import { ELOOP } from 'constants';
import * as Generated from '../deadfish_generated';

const DEBUG_CAMERA = true;

export default class Title extends Phaser.State {
    cursors = null;

    public mouseHandler(e) {
        console.log(e.event.x+this.game.camera.x, e.event.y+this.game.camera.y)
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
