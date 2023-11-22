; ModuleID = 'jit_init.c'
%struct.table_column_info = type { [128 x i8], i32, i32 }
; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @strcmp(i8* noundef %0, i8* noundef %1) #0 {
  %3 = alloca i8*, align 8
  %4 = alloca i8*, align 8
  store i8* %0, i8** %3, align 8
  store i8* %1, i8** %4, align 8
  br label %5

5:                                                ; preds = %21, %2
  %6 = load i8*, i8** %3, align 8
  %7 = load i8, i8* %6, align 1
  %8 = sext i8 %7 to i32
  %9 = load i8*, i8** %4, align 8
  %10 = load i8, i8* %9, align 1
  %11 = sext i8 %10 to i32
  %12 = icmp eq i32 %8, %11
  br i1 %12, label %13, label %18

13:                                               ; preds = %5
  %14 = load i8*, i8** %3, align 8
  %15 = load i8, i8* %14, align 1
  %16 = sext i8 %15 to i32
  %17 = icmp ne i32 %16, 0
  br label %18

18:                                               ; preds = %13, %5
  %19 = phi i1 [ false, %5 ], [ %17, %13 ]
  br i1 %19, label %20, label %26

20:                                               ; preds = %18
  br label %21

21:                                               ; preds = %20
  %22 = load i8*, i8** %3, align 8
  %23 = getelementptr inbounds i8, i8* %22, i32 1
  store i8* %23, i8** %3, align 8
  %24 = load i8*, i8** %4, align 8
  %25 = getelementptr inbounds i8, i8* %24, i32 1
  store i8* %25, i8** %4, align 8
  br label %5, !llvm.loop !6

26:                                               ; preds = %18
  %27 = load i8*, i8** %3, align 8
  %28 = load i8, i8* %27, align 1
  %29 = zext i8 %28 to i32
  %30 = load i8*, i8** %4, align 8
  %31 = load i8, i8* %30, align 1
  %32 = zext i8 %31 to i32
  %33 = sub nsw i32 %29, %32
  ret i32 %33
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local %struct.table_column_info* @query_table_column(%struct.table_column_info* noundef %0, i32 noundef %1, i8* noundef %2) #0 {
  %4 = alloca %struct.table_column_info*, align 8
  %5 = alloca %struct.table_column_info*, align 8
  %6 = alloca i32, align 4
  %7 = alloca i8*, align 8
  %8 = alloca i32, align 4
  %9 = alloca %struct.table_column_info*, align 8
  store %struct.table_column_info* %0, %struct.table_column_info** %5, align 8
  store i32 %1, i32* %6, align 4
  store i8* %2, i8** %7, align 8
  store i32 0, i32* %8, align 4
  br label %10

10:                                               ; preds = %28, %3
  %11 = load i32, i32* %8, align 4
  %12 = load i32, i32* %6, align 4
  %13 = icmp ult i32 %11, %12
  br i1 %13, label %14, label %31

14:                                               ; preds = %10
  %15 = load %struct.table_column_info*, %struct.table_column_info** %5, align 8
  %16 = load i32, i32* %8, align 4
  %17 = zext i32 %16 to i64
  %18 = getelementptr inbounds %struct.table_column_info, %struct.table_column_info* %15, i64 %17
  store %struct.table_column_info* %18, %struct.table_column_info** %9, align 8
  %19 = load i8*, i8** %7, align 8
  %20 = load %struct.table_column_info*, %struct.table_column_info** %9, align 8
  %21 = getelementptr inbounds %struct.table_column_info, %struct.table_column_info* %20, i32 0, i32 0
  %22 = getelementptr inbounds [128 x i8], [128 x i8]* %21, i64 0, i64 0
  %23 = call i32 @strcmp(i8* noundef %19, i8* noundef %22)
  %24 = icmp eq i32 %23, 0
  br i1 %24, label %25, label %27

25:                                               ; preds = %14
  %26 = load %struct.table_column_info*, %struct.table_column_info** %9, align 8
  store %struct.table_column_info* %26, %struct.table_column_info** %4, align 8
  br label %32

27:                                               ; preds = %14
  br label %28

28:                                               ; preds = %27
  %29 = load i32, i32* %8, align 4
  %30 = add i32 %29, 1
  store i32 %30, i32* %8, align 4
  br label %10, !llvm.loop !8

31:                                               ; preds = %10
  store %struct.table_column_info* null, %struct.table_column_info** %4, align 8
  br label %32

32:                                               ; preds = %31, %25
  %33 = load %struct.table_column_info*, %struct.table_column_info** %4, align 8
  ret %struct.table_column_info* %33
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i64 @get_table_column_int64(%struct.table_column_info* noundef %0, i8* noundef %1) #0 {
  %3 = alloca %struct.table_column_info*, align 8
  %4 = alloca i8*, align 8
  %5 = alloca i8*, align 8
  store %struct.table_column_info* %0, %struct.table_column_info** %3, align 8
  store i8* %1, i8** %4, align 8
  %6 = load i8*, i8** %4, align 8
  %7 = load %struct.table_column_info*, %struct.table_column_info** %3, align 8
  %8 = getelementptr inbounds %struct.table_column_info, %struct.table_column_info* %7, i32 0, i32 2
  %9 = load i32, i32* %8, align 4
  %10 = zext i32 %9 to i64
  %11 = getelementptr inbounds i8, i8* %6, i64 %10
  store i8* %11, i8** %5, align 8
  %12 = load i8*, i8** %5, align 8
  %13 = bitcast i8* %12 to i64*
  %14 = load i64, i64* %13, align 8
  ret i64 %14
}



!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
