class Factorial{
	static int factorial(int n){
		if(n == 0) return 1;
		else return factorial(n - 1) * n;
	}
	static int factorial_iter(int n){
		int i, ret = 1;
		for(;n > 0;n --)
			ret = ret * n;
		return ret;
	}
	static int factorial_neg(int n){
		if(n == 0) return 1;
		else return factorial(n - 1) * -n;
	}
}
