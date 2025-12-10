import glob
import re
import io
import sys

def file_ops(file_str, op_str):
  if (op_str == "DSP_OP"):
    op_re = (op_str + 
      r"\s*"
      r"\(\s*"
      r"(?P<name>\w+)"
      r"\s*,\s*"
      r"(?P<src_type>.*[\w\*])"
      r"\s*,\s*"
      r"(?P<dest_type>.*[\w\*])"
      r"\s*\)")
  else:
    return None

  #TODO: improve backslash handling, make sure "name" is a valid function name
  char_re = r"'[\w]'"
  str_re = r'"([\w\s\(\)\[\]\{\}:;-]|(?:\\' + '["nrt]' + r'))*?"'
  info_comment_re = (r"//[ ]*"
    r"\[[ ]*"
    r"(?P<params>"
    r"(?:[ ]*[a-zA-Z_](?:\w*[ ]*)*" # param name
    r":[ ]*"
    r"(?:\w+[ ]*)+\**[ ]*,?)" # param type
    r"+)" # end params
    r"[ ]*\][ ]*\n[ \n]*"
    + op_re)
  composite_re = "|".join((char_re,str_re,info_comment_re))
  params_re = (r"[ ]*(?P<name>[a-zA-Z_](?:[ ]*\w+)*)"
               r"[ ]*:[ ]*"
               r"(?P<type>(?:\w+[ ]*)+\**)")
  file_tokens = re.finditer(composite_re, file_str, re.M)
  ops = list()
  for m in file_tokens:
    if (m.groups() == ()) or (m.groupdict()['params'] is None):
      continue
    #if ('params' not in m.groupdict().keys()) or ('name' not in m.groupdict().keys()):
      #raise IndexError("Bad match: {}".format(m.string))
    param_tokens = re.finditer(params_re, m['params'])
    params_list = [x['type'] + ' ' + x['name'] for x in param_tokens]
    if (op_str == "DSP_OP"):
      op_sig = [m['src_type'] + " src", m['dest_type'] + " dest", "unsigned int n_bytes"]

    # clean up extra spaces
    for i, p in enumerate(params_list):
      while p.count(' '*2) > 0:
        p = p.replace(' '*2, ' ')
      while p.count(' *') > 0:
        p = p.replace(' *', '*')
      params_list[i] = p.strip()

    for i, p in enumerate(op_sig):
      while p.count(' '*2) > 0:
        p = p.replace(' '*2, ' ')
      while p.count(' *') > 0:
        p = p.replace(' *', '*')
      op_sig[i] = p.strip()

    ops.append((m['name'], params_list, op_sig))
  return ops

def discover_ops(file_glob="**/*.c", exclude_files=tuple(), 
    op_str={'src':"SRC_OP",'dsp':"DSP_OP",'sink':"SINK_OP"}):
  op_dict = {'src': list(), 'dsp': list(), 'sink': list()}
  for op_type in op_dict.keys():
    ops = list()
    op_files = list(set(glob.glob(op_type + '/' + file_glob, recursive=True)) - set(exclude_files))
    for fname in op_files:
      with open(fname) as f:
        file_str = f.read()
      ops.extend(file_ops(file_str, op_str[op_type]))
    op_dict[op_type] = ops
  return op_dict

def gen_op_wrappers(ops, in_files=["ops.h.in", "ops_internal.h.in", "ops.c.in"], out_files=["ops.h", "ops_internal.h", "ops.c"], out_dir=None):
  if (out_dir != None) and (out_dir[-1] != '/'):
    out_dir += '/'

  op_wrapper_defs = list()
  op_wrapper_decls = list()
  op_struct_decls = list()
  op_struct_defs = list()
  op_impl_defs = list()
  op_names = "{\n"
  op_wrapper_kwargs = list()
  op_wrapper_kwarg_macros = list()
  ops_h_file = io.StringIO()
  ops_internal_h_file = io.StringIO()
  ops_c_file = io.StringIO()

  for op in ops:
    op_wrapper_defs.append("int " + op[0] + "(" + ", ".join(op[2]) + ", " + ", ".join(op[1]) + ");")
    op_impl_defs.append("int " + op[0] + "_impl(" + ", ".join(op[2]) + ", " + "void *kwargs);")
    new_op_decl = "int " + op[0] + "(" + ", ".join(op[2]) + ", " + ", ".join(op[1]) + ") {\n  " + op[0] + "_kwargs_t op_args;\n"
    new_op_struct_decl = "hm_dsp_op* " + op[0] + "_op(hm_format_type input_type, hm_format_type output_type, " + ", ".join(op[1]) + ");"
    new_op_struct_def = "hm_dsp_op* " + op[0] + "_op(hm_format_type input_type, hm_format_type output_type, " + ", ".join(op[1]) + ") {\n"
    new_op_struct_def += op[0] + "_kwargs_t* op_args = malloc(sizeof(" + op[0] + "_kwargs_t));\n"
    new_op_struct_def += "  hm_dsp_op* new_op = (hm_dsp_op*)malloc(sizeof(hm_dsp_op));\n"
    new_op_struct_def += "  new_op->op_fn = {}_impl;\n".format(op[0])
    new_op_struct_def += "  new_op->input_type = input_type;\n  new_op->input_type = input_type;\n"
    op_struct_decls.append(new_op_struct_decl)
    op_wrapper_macro_str = "#define __" + op[0].upper() + "_ARGS_UNPACK"
    for param in op[1]:
      new_op_decl += "  op_args.{0} = {0};\n".format(param.split(" ")[-1])
      new_op_struct_def += "  op_args->{0} = {0};\n".format(param.split(" ")[-1])
      op_wrapper_macro_str += (" {} = ((" + op[0] + "_kwargs_t*)kwargs)->{};").format(param, param.split(" ")[-1])
    new_op_decl += "  " + op[0] + "_impl(" + ", ".join([x.split(" ")[-1] for x in op[2]]) + ", &op_args);\n}"
    new_op_struct_def += "  new_op->kwargs = (void*)op_args;\n"
    new_op_struct_def += "  return new_op;\n}"
    op_struct_defs.append(new_op_struct_def)
    op_wrapper_macro_str
    op_wrapper_decls.append(new_op_decl)
    op_wrapper_kwarg_macros.append(op_wrapper_macro_str)
    op_wrapper_kwargs.append("typedef struct {" + "; ".join(op[1]) + ";} " + op[0] + "_kwargs_t;")
    op_names += ('  "' + op[0] + '",\n')
  op_names += "  NULL\n};"

  # generate ops.h
  ops_h_file.write("// This file was generated ")
  ops_h_file.write("automatically with util/gen_ops.py\n\n")
  with open(in_files[0]) as f:
    ops_h_in_str = f.read()
  ops_h_file.write(ops_h_in_str.replace("%%OP_WRAPPER_DEFS%%", "\n".join(op_wrapper_defs + [""] + op_struct_decls)))
  ops_h_file.seek(0)
  if out_files[0] == None:
    sys.stdout.write("--- ops.h ---\n")
    sys.stdout.write(ops_h_file.read())
  elif out_dir != None:
    with open(out_dir + out_files[0], "w") as f:
      f.write(ops_h_file.read())
  else:
    with open(out_files[0], "w") as f:
      f.write(ops_h_file.read())

  # generate ops_internal.h
  ops_internal_h_file.write("// This file was generated ")
  ops_internal_h_file.write("automatically with util/gen_ops.py\n\n")
  with open(in_files[1]) as f:
    ops_internal_h_str = f.read()
  ops_internal_h_file.write(ops_internal_h_str.replace("%%OP_KWARG_DEFS%%", "\n".join(op_wrapper_kwargs))\
                                              .replace("%%OP_KWARG_MACRO_DEFS%%", "\n".join(op_wrapper_kwarg_macros))\
                                              .replace("%%OP_IMPL_DEFS%%", "\n".join(op_impl_defs)))
  ops_internal_h_file.seek(0)
  if out_files[1] == None:
    sys.stdout.write("\n--- ops_internal.h ---\n")
    sys.stdout.write(ops_internal_h_file.read())
  elif out_dir != None:
    with open(out_dir + out_files[1], "w") as f:
      f.write(ops_internal_h_file.read())
  else:
    with open(out_files[1], "w") as f:
      f.write(ops_internal_h_file.read())

  # generate ops.c
  ops_c_file.write("// This file was generated ")
  ops_c_file.write("automatically with util/gen_ops.py\n\n")
  with open(in_files[2]) as f:
    ops_c_str = f.read()
  ops_c_file.write(ops_c_str.replace("%%N_OPS%%", str(len(ops) + 1))\
                            .replace("%%OP_LIST%%", op_names) \
                            .replace("%%OP_WRAPPER_DECLS%%", "\n\n".join(op_wrapper_decls + op_struct_defs)))
  ops_c_file.seek(0)
  if out_files[2] == None:
    sys.stdout.write("\n--- ops.c ---\n")
    sys.stdout.write(ops_c_file.read())
  elif out_dir != None:
    with open(out_dir + out_files[2], "w") as f:
      f.write(ops_c_file.read())
  else:
    with open(out_files[2], "w") as f:
      f.write(ops_c_file.read())

  return None

def gen_py_op_wrappers():
  py_ops_file = io.StringIO()
  return None

if __name__ == "__main__":
  import argparse
  import itertools
  parser = argparse.ArgumentParser()
  parser.add_argument('-l', '--list', action='store_true')
  parser.add_argument('-d', '--dry-run', action='store_true')
  parser.add_argument('-o', '--out-dir', action='store', nargs=1)
  args = parser.parse_args(sys.argv[1:])
  ops_dict = discover_ops()
  ops = list(itertools.chain(*ops_dict.values()))
  if args.list:
    print(ops_dict)
  elif args.dry_run:
    gen_op_wrappers(ops,out_files=[None]*3)
  elif args.out_dir != None:
    gen_op_wrappers(ops, out_dir=args.out_dir[0])
  else:
    gen_op_wrappers(ops)
