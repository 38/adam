class Method{
	int x;
	public Method(int x){
		this.x = x;
	}
	public static void modify(Method m, int x){
		m.x = x;
	}
	public static Method run(){
		Method m = new Method(0);
		Method.modify(m, 3);
		return m;
	}
}
