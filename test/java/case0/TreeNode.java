/**
 * this is a simple BST class
 **/
class TreeNode{
	int val;
	TreeNode left, right;
	public TreeNode(int val){
		this.val = val;
	}
	public void insert(int val){
		if(this.val < val) {
			if(this.left != null) this.left.insert(val);
			else this.left = new TreeNode(val);
		} else {
			if(this.right != null) this.right.insert(val);
			else this.right = new TreeNode(val);
		}
	}
	public boolean contains(int val){
		if(this.val == val) return true;
		if(this.val < val){
			if(this.left != null) return this.left.contains(val);
			else return false;
		} else {
			if(this.right != null) return this.right.contains(val);
			else return false;
		}
	}
	public static TreeNode buildTree(){
		int i;
		TreeNode tree = new TreeNode(50);
		for(i = 1; i < 100; i ++)
			tree.insert(i);
		return tree;
	}
	public static boolean query(int x){
		TreeNode tree = buildTree();
		return tree.contains(x);
	}
}
