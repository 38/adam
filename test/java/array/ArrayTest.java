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
