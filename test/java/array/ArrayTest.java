class ArrayTest{
	int value;
	public ArrayTest(int _value) {
		this.value = _value;
	}
	public static ArrayTest[] testCreate() {
		return new ArrayTest[10];
	}
	public static ArrayTest[][] testCreate2D() {
		return new ArrayTest[5][5];
	}
	public static int[] testPut() {
		int[] array = new int[10];
		for(int i = 0; i < 10; i ++)
			array[i] = i;
		return array;
	}
	public static int getOne(int x) {
		int[] ar = testPut();
		return ar[x];
	}
	public static void factRecur(int current, int N, ArrayTest[] result){
		if(current == N) return;
		result[current] = new ArrayTest(result[current - 1].value + result[current - 2].value);
	}
	public static ArrayTest[] result;
	public static ArrayTest[] factArray(int N){
		ArrayTest[] ret = new ArrayTest[N];
		ret[0] = new ArrayTest(1);
		ret[1] = new ArrayTest(1);
		factRecur(2, N, ret);
		return ret;
	}
	public static int queryFact(int N){
		if(null == ArrayTest.result) result = factArray(100);
		return result[N].value;
	}
	public static int[] fillArray1() {
		int[] ret = {1,2,3,4};
		return ret;
	}
	public static int[] fillArray2() {
		int[] ret = {0,1,2,3,4};
		return ret;
	}
	public static int[] fillArray3() {
		int[] ret = {-1,0,1,2,3,4};
		return ret;
	}
}
