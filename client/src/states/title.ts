import * as Assets from '../assets';

export default class Title extends Phaser.State {
    public create(): void {
        this.game.camera.flash(0x000000, 1000);

        this.game.add.sprite(0, 0, Assets.Images.ImagesFish001.getName());
    }
}
