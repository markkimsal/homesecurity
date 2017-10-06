
#ifndef util_h
#define util_h
void print_hex(int v, int num_places);
void print_binary(int v, int num_places);
void print_unknown_json(char cbuf[], int len = -1);
void debug_cbuf(char cbuf[], int *idx, bool clear);
char kpaddr_to_bitmask(int kpadr);
int fetch_kpaddr();
bool store_kpaddr(int addr);
void write_sys_info();


#endif
