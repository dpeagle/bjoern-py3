#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#define NOP do{}while(0)
#define PyFile_IncUseCount(file) NOP
#define PyFile_DecUseCount(file) NOP
