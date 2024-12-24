
; LLVM IL code for the Strange Forth run-time.

; We have the main definition of the run-time's value type, supporting functions, and the run-time's
; data stack and it's supporting functions.


; The run-time's reference counted data type, how the raw data pointer is interpreted is based on
; the host Value's type.
%struct.ValueReferenceData = type
{
    i64,        ; The reference count.
    i8*         ; The data pointer.
}



; The run-time's basic value type.
%struct.Value = type
{
    i64,         ; The type of the value.
    [ 16 x i8 ]  ; The data value itself, can be either a pointer, a bool, an int64 or a f64.
}



; The run-time's data stack.
@GlobalDataStack = global [100 x %struct.Value] zeroinitializer, align 8

; Pointer into the runtime stack, we also declare the maximum stack size, the stack is empty if the
; pointer is -1.
@StackPointer = global i64 -1, align 8

@MaxStack = global i64 100, align 8



; Deep copy a value, this is used when we need to copy a value that contains a pointer to a string
; or some other sort of data structure and we need both values to have a unique copy of the data.
declare void @_deep_copy_value(%struct.Value*, %struct.Value*)



; Increment and decrement the internal reference count of a value, this is used when we need to
; shallow copy a value, and we want to keep track of how many references there are to the data.
;
; Defined externally in the run-time's C++ code.
declare void @_inc_value_reference(%struct.Value*)



; Decrement the internal reference count of a value, and free the data if the reference count is 0.
;
; Defined externally in the run-time's C++ code.
declare void @_dec_value_reference(%struct.Value*)



; Push a value onto the data stack.  The value data is shallow copied.  Return 0 on success, 1 on
; stack overflow.
define i8 @push_value(%struct.Value* %value)
{
    entry:
        ; Get the current stack information.
        %index = load i64, i64* @StackPointer, align 8
        %max = load i64, i64* @MaxStack, align 8

        %max_pre_push_index = sub i64 %max, 1

        ; Do a signed check to see if the stack is full.
        %cmp = icmp slt i64 %index, %max_pre_push_index
        br i1 %cmp, label %push, label %error

    push:
        ; Increment the values internal reference count, if it's a data time that has one.
        call void @_inc_value_reference(%struct.Value* %value)

        ; Increment the stack pointer to the next available slot.
        %new_index = add i64 %index, 1
        store i64 %new_index, i64* @StackPointer, align 8

        ; Get the base pointer to the new current item on the stack.
        %base_ptr = getelementptr inbounds [100 x %struct.Value],
                                           [100 x %struct.Value]* @GlobalDataStack,
                                           i64 0,
                                           i64 %new_index

        ; Get the addresses to the source type and data.
        %source_type_ptr = getelementptr inbounds %struct.Value,
                                                  %struct.Value* %value,
                                                  i32 0,
                                                  i32 0

        %source_data_ptr = getelementptr inbounds %struct.Value,
                                                  %struct.Value* %value,
                                                  i32 0,
                                                  i32 1

        ; Load the type and data from the source value.
        %source_type = load i64, i64* %source_type_ptr, align 8
        %source_data = load [ 16 x i8 ], [ 16 x i8 ]* %source_data_ptr, align 8

        ; Get the addresses of the destination type and data.
        %dest_type_ptr = getelementptr inbounds %struct.Value,
                                                %struct.Value* %base_ptr,
                                                i32 0,
                                                i32 0

        %dest_data_ptr = getelementptr inbounds %struct.Value,
                                                %struct.Value* %base_ptr,
                                                i32 0,
                                                i32 1

        ; Copy the type and data to the stack.
        store i64 %source_type, i64* %dest_type_ptr, align 8
        store [ 16 x i8 ] %source_data, [ 16 x i8 ]* %dest_data_ptr, align 8

        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}



; Pop a value from the data stack, again the value data is shallow copied.  Return 0 on success, 1
; on stack underflow.
define i8 @pop_value(%struct.Value* %output_value)
{
    entry:
        ; Check to see if the stack is empty, that is the stack pointer > -1.
        %index = load i64, i64* @StackPointer, align 8

        %cmp = icmp sgt i64 %index, -1
        br i1 %cmp, label %pop, label %error

    pop:
        ; Get the base pointer to the current item on the stack.
        %base_ptr = getelementptr inbounds [100 x %struct.Value],
                                           [100 x %struct.Value]* @GlobalDataStack,
                                           i64 0,
                                           i64 %index

        ; Now that the value is being popped off the stack, decrement the internal reference count.
        call void @_dec_value_reference(%struct.Value* %base_ptr)

        ; Get the base pointers to the current item on the stack.
        %source_type_ptr = getelementptr inbounds %struct.Value,
                                                  %struct.Value* %base_ptr,
                                                  i32 0,
                                                  i32 0

        %source_data_ptr = getelementptr inbounds %struct.Value,
                                                  %struct.Value* %base_ptr,
                                                  i32 0,
                                                  i32 1

        ; Get the type and data from the source value.
        %src_type = load i64, i64* %source_type_ptr, align 8
        %src_data = load [ 16 x i8 ], [ 16 x i8 ]* %source_data_ptr, align 8

        ; Get the destination pointers.
        %dest_type_ptr = getelementptr inbounds %struct.Value,
                                                %struct.Value* %output_value,
                                                i32 0,
                                                i32 0

        %dest_data_ptr = getelementptr inbounds %struct.Value,
                                                %struct.Value* %output_value,
                                                i32 0,
                                                i32 1

        ; Copy the type and data to the output value.
        store i64 %src_type, i64* %dest_type_ptr, align 8
        store [ 16 x i8 ] %src_data, [ 16 x i8 ]* %dest_data_ptr, align 8

        ; Decrement the stack pointer.
        %new_index = sub i64 %index, 1
        store i64 %new_index, i64* @StackPointer, align 8

        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}



; Useful stack manipulation functions.



; Duplicate the top value on the stack.
define i8 @dup()
{
    entry:
        ; Allocate space for the top value.
        %top_value = alloca %struct.Value

        ; Try to pop the top stack value.
        %pop_result = call i8 @pop_value(%struct.Value* %top_value)

        %cmp1 = icmp eq i8 %pop_result, 0
        br i1 %cmp1, label %push_1, label %error

    push_1:
        ; Push the top value back onto the stack.
        %push_1_result = call i8 @push_value(%struct.Value* %top_value)

        %cmp2 = icmp eq i8 %push_1_result, 0
        br i1 %cmp2, label %push_2, label %error

    push_2:
        ; Push the top value back onto the stack one more time.
        %push_2_result = call i8 @push_value(%struct.Value* %top_value)

        %cmp3 = icmp eq i8 %push_2_result, 0
        br i1 %cmp3, label %success, label %error

    success:
        ret i8 0

    error:
        ret i8 1
}
