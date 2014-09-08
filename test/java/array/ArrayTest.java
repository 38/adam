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
}
