/** @file my_macro_abuse.h
 *  @brief Macros for loop iteration, switching, and control flow.
 */
#ifndef MY_MACRO_ABUSE_H_
#define MY_MACRO_ABUSE_H_

/** @def defer_switch(...)
 *  @brief Execute some code after a matching switch case (use `continue` instead of `break`).
 *  @param ...  Switch expression. */
#ifndef defer_switch
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define defer_switch(...) \
	switch ( __VA_ARGS__ ) while (1)
#  else
#    define defer_switch(expr) \
	switch ( expr ) while (1)
#  endif
#endif

/** @def BLOCK
 *  @brief Allows early break from a block scope. */
#ifndef BLOCK
#  define BLOCK  switch (0) default:
#endif

/** @def forange(n_, start_, ...)
 *  @brief Iterate from `start_`, incrementing by 1, until `n_` < end is false.
 *  @param n_      Loop variable name (a size_t integer).
 *  @param start_  Starting value.
 *  @param ...     Exclusive end value. */
#ifndef forange
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define forange(n_, start_, ...) \
	for (size_t n_ = (start_); (n_) < (__VA_ARGS__); (n_) += 1)
#  else
/* C89 compat: user must declare `size_t n_` before using */
#    define forange(n_, start_, end_) \
	for (n_ = (start_); (n_) < (end_); (n_) += 1)
#  endif
#endif /* !forange */

/** @def leach(T_, name_, ...)
 *  @brief Iterate over a linked list by following `->next` pointers.
 *  @param T_     Node type.
 *  @param name_  Loop variable name (pointer to `T_`).
 *  @param ...    Pointer to the head node. */
#ifndef leach
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define leach(T_, name_, ...) \
	for (T_ *name_ = (__VA_ARGS__); (name_) != NULL; (name_) = (name_)->next)
#  else
/* C89 compat: user must declare `T_ *name_` before using */
#    define leach(T_, name_, head_) \
	for (name_ = (head_); (name_) != NULL; (name_) = (name_)->next)
#  endif
#endif /* !leach */

/** @def iarreach(index__, array__)
 *  @brief Iterate over an array-like struct by index.
 *  The struct must have a `.len` field of type `size_t`.
 *  @param index__  Loop variable name (`size_t`).
 *  @param array__  The array struct. */
#ifndef iarreach
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define iarreach(index__, array__) \
	for (size_t index__ = 0; (index__) < (array__).len; (index__) += 1)
#  else
/* C89 compat: user must declare `size_t index__` before using */
#    define iarreach(index__, array__) \
	for (index__ = 0; (index__) < (array__).len; (index__) += 1)
#  endif
#endif /* !iarreach */

/** @def arreach(type_, var_, arr_)
 *  @brief Iterate over an array-like struct by value.
 *  The struct must have `.items` and `.len` fields.
 *  At each iteration `var_` is a copy of the current element.
 *  @param type_  Element type.
 *  @param var_   Loop variable name.
 *  @param arr_   The array struct. */
#ifndef arreach
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define arreach(type_, var_, arr_) \
	for (type_ *var_##_ptr_ = (arr_).items, \
	           *var_##_tmp_ = (void*)1, \
	           *var_##_end_ = (arr_).items + (arr_).len; \
	           var_##_ptr_ < var_##_end_; \
	           var_##_ptr_ += 1, var_##_tmp_ = (void*)1) \
	    for (type_ var_ = *var_##_ptr_; var_##_tmp_; var_##_tmp_ = NULL)
#  else
/* C89 compat: user must declare variables before using:
   type_ *var__ptr_, *var__end_; type_ var_; */
#    define arreach(type_, var_, arr_) \
	for (var_##_ptr_ = (arr_).items, \
	           var_##_tmp_ = (void*)1, \
	           var_##_end_ = (arr_).items + (arr_).len; \
	           var_##_ptr_ < var_##_end_; \
	           var_##_ptr_ += 1, var_##_tmp_ = (void*)1) \
	    for (var_ = *var_##_ptr_; var_##_tmp_; var_##_tmp_ = NULL)
#  endif
#endif /* !arreach */

#endif /* !MY_MACRO_ABUSE_H_ */
