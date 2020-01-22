export default class WebSocketService {
    private webSocket: WebSocket = null;

    public static instance: WebSocketService = new WebSocketService();

    public init(s: string) {
        this.webSocket = new WebSocket(s);
    }

    public getWebSocket(): WebSocket {
        return this.webSocket;
    }
}