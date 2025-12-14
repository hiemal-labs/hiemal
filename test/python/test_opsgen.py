import unittest

# unit under test
import gen_ops

__all__ = ["TestOpGen"]

class TestOpGen(unittest.TestCase):
  _test_name = "gen_ops"
  def test_opsgen(self):
    test_op = {
      "name": "test_op",
      "params": {"a": "int", "b": "void*"}
    }
    test_op_wrapper = gen_ops._gen_wrapper("dsp", test_op)
    self.assertTrue(test_op_wrapper["kwargs_typedef"] == \
      "typedef struct { int a; void* b; } test_op_kwargs_t;")
    self.assertTrue(test_op_wrapper["op_wrapper_decl"] == \
      "int hm_test_op(void *src, void *dest, unsigned int n_bytes, int a, void* b);")
  
if __name__ == "__main__":
  unittest.main()