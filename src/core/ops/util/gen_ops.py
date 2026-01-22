#!/usr/bin/env python3

import io
import json
import os
import sys

def _gen_wrapper(op_type, op):
  kwargs_typedef_str = "typedef struct {{ {}; }} {}_{}_kwargs_t;"
  state_typedef_str = "typedef struct {{ {}; }} {}_{}_state_t;"
  op_kwargs_macro_str = "#define __{}_{}_ARGS_UNPACK(name) {}"
  op_state_macro_str = "#define __{}_{}_STATE_UNPACK(name) {}"
  op_wrapper_decl_str = \
    {"source":
      "int hm_source_{}(void *dest, unsigned int n_bytes, {});",
      "sink":
      "int hm_sink_{}(void *src, unsigned int n_bytes, {});",
      "dsp":
      "int hm_dsp_{}(void *src, void *dest, unsigned int n_bytes, {});"}
  op_wrapper_def_str = \
    {"source":
"""
int hm_source_{1}(void *dest, unsigned int n_bytes, {0}) {{
  {1}_{3}_kwargs_t op_args;
{2}
  {1}_{3}_state_t op_state;
  {1}_{3}_init(&op_state, &op_args);
  int rc = {1}_source_impl(dest, n_bytes, &op_args, &op_state);
  {1}_{3}_fini(&op_state, &op_args);
  return rc;
}}
""",
      "sink":
"""
int hm_sink_{1}(void *src, unsigned int n_bytes, {0}) {{
  {1}_{3}_kwargs_t op_args;
{2}
  {1}_{3}_state_t op_state;
  {1}_{3}_init(&op_state, &op_args);
  int rc = {1}_sink_impl(src, n_bytes, &op_args, &op_state);
  {1}_{3}_fini(&op_state, &op_args);
  return rc;
}}
""",
      "dsp":
"""
int hm_dsp_{1}(void *src, void *dest, unsigned int n_bytes, {0}) {{
  {1}_{3}_kwargs_t op_args;
{2}
  {1}_{3}_state_t op_state;
  {1}_{3}_init(&op_state, &op_args);
  int rc = {1}_dsp_impl(src, dest, n_bytes, &op_args, &op_state);
  {1}_{3}_fini(&op_state, &op_args);
  return rc;
}}
"""}
  op_struct_wrapper_decl_str = \
    {"source": "hm_source_op* hm_source_{}_op(hm_format_signature output_type, {});",
     "sink": "hm_sink_op* hm_sink_{}_op(hm_format_signature input_type, {});",
     "dsp": "hm_dsp_op* hm_dsp_{}_op(hm_format_signature input_type, hm_format_signature output_type, {});"}
  op_struct_wrapper_def_str = \
    {"source":
"""
hm_source_op* hm_source_{0}_op(hm_format_signature output_type, {1}) {{
  {0}_source_kwargs_t* op_args = malloc(sizeof({0}_source_kwargs_t));
  hm_source_op* new_op = (hm_source_op*)malloc(sizeof(hm_source_op));
  new_op->op_fn = {0}_source_impl;
  new_op->bytes_readable_fn = {0}_bytes_readable;
  new_op->init = {0}_source_init;
  new_op->fini = {0}_source_fini;
  new_op->output_type = output_type;
{2}
  new_op->kwargs = (void*)op_args;
{3}
  (new_op->init)(new_op->state, new_op->kwargs);
  return new_op;
}}
""",
     "sink":
"""
hm_sink_op* hm_sink_{0}_op(hm_format_signature input_type, {1}) {{
  {0}_sink_kwargs_t* op_args = malloc(sizeof({0}_sink_kwargs_t));
  hm_sink_op* new_op = (hm_sink_op*)malloc(sizeof(hm_sink_op));
  new_op->op_fn = {0}_sink_impl;
  new_op->init = {0}_sink_init;
  new_op->fini = {0}_sink_fini;
  new_op->bytes_writable_fn = {0}_bytes_writable;
  new_op->input_type = input_type;
{2}
  new_op->kwargs = (void*)op_args;
{3}
  (new_op->init)(new_op->state, new_op->kwargs);
  return new_op;
}}
""",
     "dsp":
"""
hm_dsp_op* hm_dsp_{0}_op(hm_format_signature input_type, hm_format_signature output_type, {1}) {{
  {0}_dsp_kwargs_t* op_args = malloc(sizeof({0}_dsp_kwargs_t));
  hm_dsp_op* new_op = (hm_dsp_op*)malloc(sizeof(hm_dsp_op));
  new_op->op_fn = {0}_dsp_impl;
  new_op->init = {0}_dsp_init;
  new_op->fini = {0}_dsp_fini;
  new_op->input_type = input_type;
  new_op->output_type = output_type;
{2}
  new_op->kwargs = (void*)op_args;
{3}
  (new_op->init)(new_op->state, new_op->kwargs);
  return new_op;
}}
""",}
  op_impl_decl_str =\
    {"source": "int {0}_{1}_impl(void* dest, unsigned int n_bytes, void* kwargs, void* state);\n"
      "int {0}_bytes_readable(hm_source_op *source_op, size_t *bytes_readable);\n"
      "int {0}_source_init(void* state, void* kwargs);\n"
      "int {0}_source_fini(void* state, void* kwargs);",
     "sink": "int {0}_{1}_impl(void* src, unsigned int n_bytes, void* kwargs, void* state);\n"
      "int {0}_bytes_writable(hm_sink_op *sink_op, size_t *bytes_writable);\n"
      "int {0}_sink_init(void* state, void* kwargs);\n"
      "int {0}_sink_fini(void* state, void* kwargs);",
     "dsp": "int {0}_{1}_impl(void* src, void* dest, unsigned int n_bytes, void* kwargs, void* state);\n"
      "int {0}_dsp_init(void* state, void* kwargs);\n"
      "int {0}_dsp_fini(void* state, void* kwargs);",}

  if "params" in op.keys() and op["params"]:
    op_param_list = '; '.join(
      ["{} {}".format(param_type, param_name) for param_name, param_type in op["params"].items()]
    )
    kwargs_typedef = kwargs_typedef_str.format(op_param_list, op["name"], op_type)
    kwargs_macro_def = '; '.join(
      ["{0} {1} = (({2}_{3}_kwargs_t*)kwargs)->{1}".format(op["params"][param_name],
      param_name, op["name"], op_type) for param_name in op["params"].keys()]
    )
    kwargs_macro_def += ';\n'
    op_wrapper_kwargs_fill = "\n".join(
      ["  op_args.{0} = {0};".format(param) for param in op["params"].keys()]
    )
    op_kwargs_macro_def = '; '.join(
      ["{0} {1} = (({2}_{3}_kwargs_t*)(name))->{1}".format(op["params"][param_name],
      param_name, op["name"], op_type) for param_name in op["params"].keys()]
    )
    op_kwargs_macro_def += ';\n'
  else:
    op_param_list = ''
    kwargs_typedef = 'typedef struct {{}} {}_{}_kwargs_t;'.format(op["name"], op_type)
    kwargs_macro_def = ''
    op_wrapper_kwargs_fill = ''
  
  if "state" in op.keys() and op["state"]:
    op_state_list = '; '.join(
      ["{} {}".format(param_type, param_name) for param_name, param_type in op["state"].items()]
    )
    state_typedef = state_typedef_str.format(op_state_list, op["name"], op_type)
    op_state_macro_def = '; '.join(
      ["{0} state_{1} = (({2}_{3}_state_t*)(name))->{1}".format(op["state"][param_name],
      param_name, op["name"], op_type) for param_name in op["state"].keys()]
    ) + ';\n'
    op_state_malloc = '  new_op->state = calloc(sizeof({}_{}_kwargs_t),1);'.format(op["name"], op_type)
  else:
    state_typedef = 'typedef struct {{}} {}_{}_state_t;'.format(op["name"], op_type)
    op_state_macro_def = ''
    op_state_malloc = 'new_op->state = NULL;'

  op_wrapper_decl = op_wrapper_decl_str[op_type].format(op["name"], op_param_list.replace(';', ','))
  new_impl_decl = op_impl_decl_str[op_type].format(op["name"], op_type)

  op_wrapper = {}
  op_wrapper["kwargs_typedef"] = kwargs_typedef
  op_wrapper["state_typedef"] = state_typedef
  op_wrapper["op_kwargs_macro_def"] = op_kwargs_macro_str.format(op_type.upper(), op["name"].upper(), op_kwargs_macro_def)
  op_wrapper["op_state_macro_def"] = op_state_macro_str.format(op_type.upper(), op["name"].upper(), op_state_macro_def)
  op_wrapper["op_wrapper_decl"] = op_wrapper_decl
  op_wrapper["op_wrapper_def"] = op_wrapper_def_str[op_type].format(op_param_list.replace(';', ','), op["name"], op_wrapper_kwargs_fill, op_type)
  op_wrapper["op_struct_wrapper_decl"] = op_struct_wrapper_decl_str[op_type].format(op["name"], op_param_list.replace(';', ','))
  op_wrapper["op_struct_wrapper_def"] = op_struct_wrapper_def_str[op_type].format(op["name"], op_param_list.replace(';', ','), op_wrapper_kwargs_fill.replace('.', '->'), op_state_malloc)
  op_wrapper["impl_decl"] = new_impl_decl

  if not ("params" in op.keys() and op["params"]):
    #op_wrapper["kwargs_macro_def"] = ''
    op_wrapper["op_wrapper_def"] = op_wrapper["op_wrapper_def"].replace(', ) {', ') {')
    op_wrapper["op_wrapper_decl"] = op_wrapper["op_wrapper_decl"].replace(', );', ');')
    op_wrapper["op_struct_wrapper_def"] = op_wrapper["op_struct_wrapper_def"].replace(', ) {', ') {')
    op_wrapper["op_struct_wrapper_decl"] = op_wrapper["op_struct_wrapper_decl"].replace(', );', ');')

  return op_wrapper

def gen_op_wrappers(ops_file, dry_run, out_dir, template_dir, gen_py_ops):
  if isinstance(out_dir, list):
    out_dir = out_dir[0]
  if isinstance(template_dir, list):
    template_dir = template_dir[0]
  if (out_dir != None) and (out_dir[-1] != '/'):
    out_dir += '/'
  with open(ops_file, 'r') as f:
    ops = json.load(f)

  kwargs_typedef_dict = {"source": [], "sink": [], "dsp": []}
  state_typedef_dict = {"source": [], "sink": [], "dsp": []}
  op_kwargs_macro_def_dict = {"source": [], "sink": [], "dsp": []}
  op_state_macro_def_dict = {"source": [], "sink": [], "dsp": []}
  op_wrapper_decl_dict = {"source": [], "sink": [], "dsp": []}
  op_wrapper_def_dict = {"source": [], "sink": [], "dsp": []}
  op_struct_wrapper_decl_dict = {"source": [], "sink": [], "dsp": []}
  op_struct_wrapper_def_dict = {"source": [], "sink": [], "dsp": []}
  impl_decl_dict = {"source": [], "sink": [], "dsp": []}

  op_name_list_str = "static const char* ops_list[{}] = {{\n".format(len(ops["source"]) + len(ops["sink"]) + len(ops["dsp"]) + 3)
  for op_type, op_list in ops.items():
    for op in op_list:
      op_name_list_str += '  "{}",\n'.format(op["name"])
      op_wrapper = _gen_wrapper(op_type, op)

      kwargs_typedef_dict[op_type].append(op_wrapper["kwargs_typedef"])
      state_typedef_dict[op_type].append(op_wrapper["state_typedef"])
      op_kwargs_macro_def_dict[op_type].append(op_wrapper["op_kwargs_macro_def"])
      op_state_macro_def_dict[op_type].append(op_wrapper["op_state_macro_def"])
      op_wrapper_decl_dict[op_type].append(op_wrapper["op_wrapper_decl"])
      op_wrapper_def_dict[op_type].append(op_wrapper["op_wrapper_def"])
      op_struct_wrapper_decl_dict[op_type].append(op_wrapper["op_struct_wrapper_decl"])
      op_struct_wrapper_def_dict[op_type].append(op_wrapper["op_struct_wrapper_def"])
      impl_decl_dict[op_type].append(op_wrapper["impl_decl"])
    op_name_list_str += "  NULL,\n"
  op_name_list_str += "};"

  # ops.h
  ops_h_file = io.StringIO("// This file was generated automatically with util/gen_ops.py\n\n")
  with open(template_dir + "/ops.h.in", "r") as f:
    ops_h_str = f.read()\
                  .replace("%%SOURCE_OP_DECL%%", "\n".join(op_wrapper_decl_dict["source"]))\
                  .replace("%%SOURCE_OP_DECL_STRUCT%%", "\n".join(op_struct_wrapper_decl_dict["source"]))\
                  .replace("%%SINK_OP_DECL%%", "\n".join(op_wrapper_decl_dict["sink"]))\
                  .replace("%%SINK_OP_DECL_STRUCT%%", "\n".join(op_struct_wrapper_decl_dict["sink"]))\
                  .replace("%%DSP_OP_DECL%%", "\n".join(op_wrapper_decl_dict["dsp"]))\
                  .replace("%%DSP_OP_DECL_STRUCT%%", "\n".join(op_struct_wrapper_decl_dict["dsp"]))
    ops_h_file.write(ops_h_str)
  ops_h_file.seek(0, os.SEEK_END)

  # ops_internal.h
  ops_internal_h_file = io.StringIO("// This file was generated automatically with util/gen_ops.py\n\n")
  with open(template_dir + "/ops_internal.h.in", "r") as f:
    ops_internal_h_str = f.read()\
                  .replace("%%SOURCE_KWARGS_TYPEDEF%%", "\n".join(kwargs_typedef_dict["source"]))\
                  .replace("%%SOURCE_STATE_TYPEDEF%%", "\n".join(state_typedef_dict["source"]))\
                  .replace("%%SOURCE_OP_KWARGS_MACRO_DEF%%", "\n".join(op_kwargs_macro_def_dict["source"]))\
                  .replace("%%SOURCE_OP_STATE_MACRO_DEF%%", "\n".join(op_state_macro_def_dict["source"]))\
                  .replace("%%SOURCE_IMPL_DECL%%", "\n".join(impl_decl_dict["source"]))\
                  .replace("%%SINK_KWARGS_TYPEDEF%%", "\n".join(kwargs_typedef_dict["sink"]))\
                  .replace("%%SINK_STATE_TYPEDEF%%", "\n".join(state_typedef_dict["sink"]))\
                  .replace("%%SINK_OP_KWARGS_MACRO_DEF%%", "\n".join(op_kwargs_macro_def_dict["sink"]))\
                  .replace("%%SINK_OP_STATE_MACRO_DEF%%", "\n".join(op_state_macro_def_dict["sink"]))\
                  .replace("%%SINK_IMPL_DECL%%", "\n".join(impl_decl_dict["sink"]))\
                  .replace("%%DSP_KWARGS_TYPEDEF%%", "\n".join(kwargs_typedef_dict["dsp"]))\
                  .replace("%%DSP_STATE_TYPEDEF%%", "\n".join(state_typedef_dict["dsp"]))\
                  .replace("%%DSP_OP_KWARGS_MACRO_DEF%%", "\n".join(op_kwargs_macro_def_dict["dsp"]))\
                  .replace("%%DSP_OP_STATE_MACRO_DEF%%", "\n".join(op_state_macro_def_dict["dsp"]))\
                  .replace("%%DSP_IMPL_DECL%%", "\n".join(impl_decl_dict["dsp"]))
    ops_internal_h_file.write(ops_internal_h_str)
  ops_internal_h_file.seek(0, os.SEEK_END)

  #ops.c
  ops_c_file = io.StringIO("// This file was generated automatically with util/gen_ops.py\n\n")
  with open(template_dir + "/ops.c.in", "r") as f:
    ops_c_str = f.read()\
                  .replace("%%SOURCE_OP_WRAPPER_DEF%%", "\n\n".join(op_wrapper_def_dict["source"]))\
                  .replace("%%SOURCE_OP_STRUCT_WRAPPER_DEF%%", "\n\n".join(op_struct_wrapper_def_dict["source"]))\
                  .replace("%%SINK_OP_WRAPPER_DEF%%", "\n\n".join(op_wrapper_def_dict["sink"]))\
                  .replace("%%SINK_OP_STRUCT_WRAPPER_DEF%%", "\n\n".join(op_struct_wrapper_def_dict["sink"]))\
                  .replace("%%DSP_OP_WRAPPER_DEF%%", "\n\n".join(op_wrapper_def_dict["dsp"]))\
                  .replace("%%DSP_OP_STRUCT_WRAPPER_DEF%%", "\n\n".join(op_struct_wrapper_def_dict["dsp"]))\
                  .replace("%%OP_NAME_LIST_STR%%", op_name_list_str)\
                  .replace("%%SINK_LIST_START%%", str(len(ops["source"])+1))\
                  .replace("%%DSP_LIST_START%%", str(len(ops["source"]) + len(ops["sink"]) + 2))                  
    ops_c_file.write(ops_c_str)
  ops_c_file.seek(0, os.SEEK_END)

  if gen_py_ops:
    # ops_python.h
    # ops_python.c
    pass


  ops_h_file.seek(0)
  ops_internal_h_file.seek(0)
  ops_c_file.seek(0)

  if dry_run:
    sys.stdout.write("--- ops.h ---\n")
    sys.stdout.write(ops_h_file.read())
    sys.stdout.write("\n--- ops_internal.h ---\n")
    sys.stdout.write(ops_internal_h_file.read())
    sys.stdout.write("\n--- ops.c ---\n")
    sys.stdout.write(ops_c_file.read())
  elif out_dir is None:
    raise ValueError("no out_dir specified")
  else:
    with open(out_dir + "ops.h", 'w') as f:
      f.write(ops_h_file.read())
    with open(out_dir + "ops_internal.h", 'w') as f:
      f.write(ops_internal_h_file.read())
    with open(out_dir + "ops.c", 'w') as f:
      f.write(ops_c_file.read())          

if __name__ == "__main__":
  import argparse
  parser = argparse.ArgumentParser()
  parser.add_argument('-l', '--list', action='store_true')
  parser.add_argument('-d', '--dry-run', action='store_true')
  parser.add_argument('-i', '--ops-file', default='ops.json', action='store', nargs=1)
  parser.add_argument('-o', '--out-dir', action='store', nargs=1)
  parser.add_argument('-t', '--template-dir', action='store', nargs=1)
  parser.add_argument('--python', action='store_true')
  args = parser.parse_args(sys.argv[1:])
  gen_op_wrappers(args.ops_file, args.dry_run, args.out_dir, args.template_dir, args.python)