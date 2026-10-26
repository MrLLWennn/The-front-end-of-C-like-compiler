// 简易完整C程序：函数调用、循环、比较、条件判断与main入口。
// 不使用头文件、标准库或输入输出；main返回0表示全部自检通过。
int square(int x)
{
    return x * x;
}

int sum_squares(int n)
{
    int i = 1;
    int total = 0;
    while(i <= n)
    {
        total = total + square(i);
        i = i + 1;
    }
    return total;
}

int main(void)
{
    int code = 0;
    if(sum_squares(5) != 55)
    {
        code = 1;
    }
    if(sum_squares(0) != 0)
    {
        code = 2;
    }
    if(sum_squares(10) != 385)
    {
        code = 3;
    }
    return code;
}
