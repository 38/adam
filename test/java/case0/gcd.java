class Gcd{
	public static int gcd(int a, int b){
		if(b == 0) return a;
		return gcd(b, a%b);
	}
	public static int gcd_iter(int a, int b){
		while(b != 0) {
			int c = a;
			a = b;
			b = c % b;
		}
		return a;
	}
}
