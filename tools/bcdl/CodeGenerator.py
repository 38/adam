from abc import abstractmethod
_symidx = 0
class CodeGenerator(object):
	"""
		The base class of any code generator
		We divide a C code in three section
			1) header (include and macros)
			2) Computation 
			3) Reference
	"""
	_data = []
	_sym = ""
	def __init__(self):
		self.init()
	def _init(self):
		global _symidx
		self._sym = "var%d"%_symidx
		self._gen_decl = False
		self._gen_comp = False
		_symidx += 1
	def generate_decl(self):
		if self._gen_decl: return ""
		self._gen_decl = True
		return "\n".join([getattr(self, child).generate_decl() for child in self._data])
	def generate_comp(self):
		if self._gen_comp: return ""
		self._gen_comp = True
		return "\n".join([getattr(self, child).generate_comp() for child in self._data])
	def generate_ref(self):
		return self._sym

class Declearation(CodeGenerator):
	"The class to generate the declreation code"
	_data = ["code"]
	def __init__(self, code):
		self.code = code
		self._init()
	def generate_decl(self):
		return self.code
	def generate_comp(self):
		return ""
	def generate_ref(self):
		return ""
dep = 0
class CSource(CodeGenerator):
	"""
		A C Code here. You can make invocation to a C code in BCDL
	"""
	_data = ["_filename"]
	def __init__(self, filename):
		self._filename = Declearation("#include <%s>"%filename)
		self._init()
	def __getattribute__(self, name):
		if name[0] == '_' or name == "generate_decl" or name == "generate_comp" or name == "generate_ref": 
			return object.__getattribute__(self,name)
		source_obj = self
		def function(*args):
			class CInvokeWrap(object):
				def __getattribute__(self, T):
					#if T[-1] == '_': T = T[:-1] + "*"
					L = 0
					for i in range(0, len(T)): 
						if T[len(T) - 1 - i] != '_': break
						L += 1
					if L: T = T[:-L] + "*" * L
					class CInvoke(CodeGenerator):
						arglist = args
						source = source_obj
						func = name
						def __init__(self):
							self._init()
							self._data = ["source"]
							for i, arg in enumerate(self.arglist):
								setattr(self, "arg%d"%i, arg)
								self._data.append("arg%d"%i)
						def generate_comp(self):
							ret = ""
							for arg in self.arglist:
								ret += arg.generate_comp() + "\n"
							ret += "%s %s = %s("%(T, self.generate_ref(), self.func)
							for arg in self.arglist:
								ret += arg.generate_ref() + ","
							if self.arglist: ret = ret[:-1] 
							ret += ");"
							return ret
					return CInvoke()
			return CInvokeWrap()
		return function


if __name__ == "__main__":
	test_h = CSource("test.h")
	invoke = test_h.foo(test_h.goo().size_t, test_h.koo(test_h.zoo().double).cesk_set_t_).int
	print invoke.generate_decl()
	print invoke.generate_comp()
	print invoke.generate_ref()
