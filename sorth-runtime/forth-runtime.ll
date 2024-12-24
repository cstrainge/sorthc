
; LLVM IL code for the Strange Forth run-time.

; We have the main definition of the run-time's value type, supporting functions, and the run-time's
; data stack and it's supporting functions.



; The run-time's basic value type.
%struct.Value = type
{
    i64,        ; The type of the value.
    [ 8 x i8 ]  ; The data value itself, can be either a pointer, a bool, an int64 or a f64.
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



; When overwriting a value, we need to free the old data which may include a pointer to a string or
; or some other sorth data structure.
;
; Defined externally in the run-time's C++ code.
declare void @_free_value_data(%struct.Value*)



; Push a value onto the data stack.  The value data is shallow copied.  Return 0 on success, 1 on
; stack overflow.
define i8 @push_value(%struct.Value* %value)
{
        ; Get the current stack information.
        %index = load i64, i64* @StackPointer, align 8
        %max = load i64, i64* @MaxStack, align 8

        %max_pre_push_index = sub i64 %max, 1

        ; Do a signed check to see if the stack is full.
        %cmp = icmp slt i64 %index, %max_pre_push_index
        br i1 %cmp, label %push, label %error

    push:
        ; Increment the stack pointer to the next available slot.
        %new_index = add i64 %index, 1
        store i64 %new_index, i64* @StackPointer, align 8

        ; Get the base pointer to the new current item on the stack.
        %base_ptr = getelementptr inbounds [100 x %struct.Value],
                                           [100 x %struct.Value]* @GlobalDataStack,
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
        store i64 %new_index, i64* @StackPointer, align 8

        ; Success!
        ret i8 0

    error:
        ; Failed!
        ret i8 1
}
