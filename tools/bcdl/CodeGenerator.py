from abc import abstractmethod
class CodeGenerator(object):
	"The base class of any code generator"
	@abstractmethod
	def __init__(self): pass
	@abstractmethod
	def generate_decl(self): pass 
	#todo
class Declearation(object):
	def __init__(self, stmt):
		self._stmt = stmt
	def generate(self):
		return (self._stmt, "", "")
def CSource(filename):
	class _CG(CodeGenerator):
		def __init__(self):
			self.data = [Declearation("#include<%s>"%filename)]
		def __getattr__(self, name):

