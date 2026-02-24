typedef struct
{
  long total_kb;
  long free_kb;
  long available_kb;
  long swap_total_kb;
  long swap_free_kb;
} MemInfo;

void get_proc_meminfo(MemInfo *info);
