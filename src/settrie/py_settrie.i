%module py_settrie

%{
	extern int new_settrie();
	extern void destroy_settrie(int st_id);
	extern void insert	(int st_id, char *set, char *str_id);
	extern char *find (int st_id, char *set);
	extern int supersets (int st_id, char *set);
	extern int subsets (int st_id, char *set);
	extern int elements (int st_id, int set_id);
	extern int next_set_id (int st_id, int set_id);
	extern char *set_name (int st_id, int set_id);
	extern int iterator_size (int iter_id);
	extern char *iterator_next (int iter_id);
	extern void destroy_iterator (int iter_id);
	extern int save_as_binary_image (int st_id);
	extern bool push_binary_image_block (int st_id, char *p_block);
	extern int binary_image_size (int image_id);
	extern char *binary_image_next (int image_id);
	extern void destroy_binary_image (int image_id);
%}

extern int new_settrie();
extern void destroy_settrie(int st_id);
extern void insert	(int st_id, char *set, char *str_id);
extern char *find (int st_id, char *set);
extern int supersets (int st_id, char *set);
extern int subsets (int st_id, char *set);
extern int elements (int st_id, int set_id);
extern int next_set_id (int st_id, int set_id);
extern char *set_name (int st_id, int set_id);
extern int iterator_size (int iter_id);
extern char *iterator_next (int iter_id);
extern void destroy_iterator (int iter_id);
extern int save_as_binary_image (int st_id);
extern bool push_binary_image_block (int st_id, char *p_block);
extern int binary_image_size (int image_id);
extern char *binary_image_next (int image_id);
extern void destroy_binary_image (int image_id);
