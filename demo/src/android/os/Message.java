package android.os;
import android.os.*;
class Message{
	Object obj;
	int what;
	Message(Object msg) {
		this.obj = msg;
	}
	public static Message obtain(Handler h, int o) {
		return new Message(o);
	}
}
