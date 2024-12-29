; LLVM IL code for the Strange Forth run-time.

; We have the main definition of the run-time's value type, supporting functions, and the run-time's
; data stack and it's supporting functions.



; Value type ids.
@type_value_none   = constant i64 0, align 8
@type_value_int    = constant i64 1, align 8
@type_value_float  = constant i64 2, align 8
@type_value_bool   = constant i64 3, align 8
@type_value_string = constant i64 4, align 8



; Holds the data for a value, this can be a pointer do a more advanced data type or, a bool, an
; i64 or a f64.
%struct.ValuePayload = type
{
    i64,  ; Reference count, or value based on the Value type.
    i8*   ; Pointer to the value.
}



; The run-time's basic value type.
%struct.Value = type
{
    i64,                  ; The type of the value.
    %struct.ValuePayload  ; The data value itself, can be either a pointer, a bool, an int64 or a
                          ; f64.
}



; The run-time's data stack.
@global_data_stack = global [100 x %struct.Value] zeroinitializer, align 8

; Pointer into the runtime stack, we also declare the maximum stack size, the stack is empty if the
; pointer is -1.
@stack_pointer = global i64 -1, align 8

@max_stack = global i64 100, align 8



; Convert an int64 into a value.
define void @value_from_int64(i64 %int_value, %struct.Value* %output_value)
{
    entry:
        ; Get the type and data pointers.
        %type_ptr = getelementptr inbounds %struct.Value,
                                           %struct.Value* %output_value,
                                           i32 0,
                                           i32 0

        %data_ptr = getelementptr inbounds %struct.Value,
                                           %struct.Value* %output_value,
                                           i32 0,
                                           i32 1

        ; Set the type and data.
        %type_id = load i64, i64* @type_value_int, align 8

        store i64 %type_id, i64* %type_ptr, align 8
        store i64 %int_value, i64* %data_ptr, align 8

        ret void
}



; Convert an int64 into a value.
define void @value_from_float64(double %float_value, %struct.Value* %output_value)
{
    entry:
        ; Get the type and data pointers.
        %type_ptr = getelementptr inbounds %struct.Value,
                                           %struct.Value* %output_value,
                                           i32 0,
                                           i32 0

        %data_ptr = getelementptr inbounds %struct.Value,
                                           %struct.Value* %output_value,
                                           i32 0,
                                           i32 1

        ; Set the type and data.
        %type_id = load i64, i64* @type_value_float, align 8

        store i64 %type_id, i64* %type_ptr, align 8
        store double %float_value, double* %data_ptr, align 8

        ret void
}



; Convert a bool into a value.
define void @value_from_bool(i8 %bool_value, %struct.Value* %output_value)
{
    entry:
        ; Get the type and data pointers.
        %type_ptr = getelementptr inbounds %struct.Value,
                                           %struct.Value* %output_value,
                                           i32 0,
                                           i32 0

        %data_ptr = getelementptr inbounds %struct.Value,
                                           %struct.Value* %output_value,
                                           i32 0,
                                           i32 1

        ; Set the type and data.
        %type_id = load i64, i64* @type_value_bool, align 8

        store i64 %type_id, i64* %type_ptr, align 8
        store i8 %bool_value, i8* %data_ptr, align 8

        ret void
}



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
        %index = load i64, i64* @stack_pointer, align 8
        %max = load i64, i64* @max_stack, align 8

        %max_pre_push_index = sub i64 %max, 1

        ; Do a signed check to see if the stack is full.
        %cmp = icmp slt i64 %index, %max_pre_push_index
        br i1 %cmp, label %push, label %error

    push:
        ; Increment the values internal reference count, if it's a data time that has one.
        call void @_inc_value_reference(%struct.Value* %value)

        ; Increment the stack pointer to the next available slot.
        %new_index = add i64 %index, 1
        store i64 %new_index, i64* @stack_pointer, align 8

        ; Get the base pointer to the new current item on the stack.
        %base_ptr = getelementptr inbounds [100 x %struct.Value],
                                           [100 x %struct.Value]* @global_data_stack,
                                           i64 0,
                                           i64 %new_index

        ; Get the type and data from the source value.
        %type_ptr =  getelementptr inbounds %struct.Value,
                                            %struct.Value* %value,
                                            i32 0,
                                            i32 0

        %data_ptr = getelementptr inbounds %struct.Value,
                                           %struct.Value* %value,
                                           i32 0,
                                           i32 1

        %type = load i64, i64* %type_ptr, align 8
        %data = load [ 8 x i8 ], [ 8 x i8 ]* %data_ptr, align 8

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
        store i64 %type, i64* %dest_type_ptr, align 8
        store [ 8 x i8 ] %data, [ 8 x i8 ]* %dest_data_ptr, align 8

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
        %index = load i64, i64* @stack_pointer, align 8

        %cmp = icmp sgt i64 %index, -1
        br i1 %cmp, label %pop, label %error

    pop:
        ; Get the base pointer to the current item on the stack.
        %base_ptr = getelementptr inbounds [100 x %struct.Value],
                                           [100 x %struct.Value]* @global_data_stack,
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
        %src_data = load [ 8 x i8 ], [ 8 x i8 ]* %source_data_ptr, align 8

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
        store [ 8 x i8 ] %src_data, [ 8 x i8 ]* %dest_data_ptr, align 8

        ; Decrement the stack pointer.
        %new_index = sub i64 %index, 1
        store i64 %new_index, i64* @stack_pointer, align 8

        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}



; Duplicate teh top value on the stack.
define i8 @dup()
{
    entry:
        ; Allocate a new value.
        %top_value = alloca %struct.Value, align 8

        ; Pop the top value from the stack.
        %pop_result = call i8 @pop_value(%struct.Value* %top_value)

        %cmp1 = icmp eq i8 %pop_result, 0
        br i1 %cmp1, label %push1, label %error

    push1:
        ; Push the top value back onto the stack.
        %push1_result = call i8 @push_value(%struct.Value* %top_value)

        %cmp2 = icmp eq i8 %push1_result, 0
        br i1 %cmp2, label %push2, label %error

    push2:
        ; Push the top value back onto the stack a second time.
        %push2_result = call i8 @push_value(%struct.Value* %top_value)

        %cmp3 = icmp eq i8 %push2_result, 0
        br i1 %cmp3, label %success, label %error

    success:
        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}



; Drop the top value from the stack.
define i8 @drop()
{
    entry:
        ; Allocate a new value.
        %top_value = alloca %struct.Value, align 8

        ; Pop the top value from the stack.
        %pop_result = call i8 @pop_value(%struct.Value* %top_value)

        %cmp = icmp eq i8 %pop_result, 0
        br i1 %cmp, label %success, label %error

    success:
        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}



; Swap the top two values on the stack.
define i8 @swap()
{
    entry:
        ; Allocate two new values.
        %first_value = alloca %struct.Value, align 8
        %second_value = alloca %struct.Value, align 8

        ; Pop the first value from the stack.
        %pop1_result = call i8 @pop_value(%struct.Value* %first_value)
        %cmp1 = icmp eq i8 %pop1_result, 0
        br i1 %cmp1, label %pop2, label %error

    pop2:
        ; Pop the second value from the stack.
        %pop2_result = call i8 @pop_value(%struct.Value* %second_value)
        %cmp2 = icmp eq i8 %pop2_result, 0
        br i1 %cmp2, label %push1, label %error

    push1:
        ; Push the second value back onto the stack.
        %push1_result = call i8 @push_value(%struct.Value* %second_value)
        %cmp3 = icmp eq i8 %push1_result, 0
        br i1 %cmp3, label %push2, label %error

    push2:
        ; Push the first value back onto the stack.
        %push2_result = call i8 @push_value(%struct.Value* %first_value)
        %cmp4 = icmp eq i8 %push2_result, 0
        br i1 %cmp4, label %success, label %error

    success:
        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}



; Pop the top two values from the stack, a, b, and then push them back as b, a, b.
define i8 @over()
{
    entry:
        ; Allocate two new values.
        %first_value = alloca %struct.Value, align 8
        %second_value = alloca %struct.Value, align 8

        ; Pop the first value from the stack.
        %pop1_result = call i8 @pop_value(%struct.Value* %first_value)
        %cmp1 = icmp eq i8 %pop1_result, 0
        br i1 %cmp1, label %pop2, label %error

    pop2:
        ; Pop the second value from the stack.
        %pop2_result = call i8 @pop_value(%struct.Value* %second_value)
        %cmp2 = icmp eq i8 %pop2_result, 0
        br i1 %cmp2, label %push1, label %error

    push1:
        ; Push the second value back onto the stack.
        %push1_result = call i8 @push_value(%struct.Value* %second_value)
        %cmp3 = icmp eq i8 %push1_result, 0
        br i1 %cmp3, label %push2, label %error

    push2:
        ; Push the first value back onto the stack.
        %push2_result = call i8 @push_value(%struct.Value* %first_value)
        %cmp4 = icmp eq i8 %push2_result, 0
        br i1 %cmp4, label %push3, label %error

    push3:
        ; Push the second value back onto the stack.
        %push3_result = call i8 @push_value(%struct.Value* %second_value)
        %cmp5 = icmp eq i8 %push3_result, 0
        br i1 %cmp5, label %success, label %error

    success:
        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}



; Rotate the top three values on the stack, a, b, c, and then push them back as c, a, b.
define i8 @rot()
{
    entry:
        ; Allocate three new values.
        %first_value = alloca %struct.Value, align 8
        %second_value = alloca %struct.Value, align 8
        %third_value = alloca %struct.Value, align 8

        ; Pop the first value from the stack.
        %pop1_result = call i8 @pop_value(%struct.Value* %first_value)
        %cmp1 = icmp eq i8 %pop1_result, 0
        br i1 %cmp1, label %pop2, label %error

    pop2:
        ; Pop the second value from the stack.
        %pop2_result = call i8 @pop_value(%struct.Value* %second_value)
        %cmp2 = icmp eq i8 %pop2_result, 0
        br i1 %cmp2, label %pop3, label %error

    pop3:
        ; Pop the third value from the stack.
        %pop3_result = call i8 @pop_value(%struct.Value* %third_value)
        %cmp3 = icmp eq i8 %pop3_result, 0
        br i1 %cmp3, label %push1, label %error

    push1:
        ; Push the second value back onto the stack.
        %push1_result = call i8 @push_value(%struct.Value* %third_value)
        %cmp4 = icmp eq i8 %push1_result, 0
        br i1 %cmp4, label %push2, label %error

    push2:
        ; Push the first value back onto the stack.
        %push2_result = call i8 @push_value(%struct.Value* %first_value)
        %cmp5 = icmp eq i8 %push2_result, 0
        br i1 %cmp5, label %push3, label %error

    push3:
        ; Push the third value back onto the stack.
        %push3_result = call i8 @push_value(%struct.Value* %second_value)
        %cmp6 = icmp eq i8 %push3_result, 0
        br i1 %cmp6, label %success, label %error

    success:
        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}



; Push the current stack depth onto the stack.
define i8 @stack_depth()
{
    entry:
        ; Allocate a new value for pushing.
        %int_value = alloca %struct.Value, align 8

        ; Get the stack pointer, and compute the depth.
        %index = load i64, i64* @stack_pointer, align 8
        %depth = add i64 %index, 1

        ; Convert the depth to a value.
        call void @value_from_int64(i64 %depth, %struct.Value* %int_value)

        ; Push the depth value onto the stack.
        %push_result = call i8 @push_value(%struct.Value* %int_value)
        %cmp = icmp eq i8 %push_result, 0
        br i1 %cmp, label %success, label %error

    success:
        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}
