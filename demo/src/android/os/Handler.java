package android.os;
import android.os.*;
class Handler {
	public void handleMessage(final Message msg) {

	}
	public boolean sendMessage(Message m) {
		this.handleMessage(m);
		return true;
	}
}
