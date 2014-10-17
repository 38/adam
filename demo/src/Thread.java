class Thread{
	Thread(java.lang.Runnable what, String name){
		what.run();
	}
}
class Runnable{
	public void run(){}
}
