<?php
// 测试 synchronize 错误恢复机制
// 本文件包含故意制造的语法错误，用于验证解析器能否正确恢复并继续解析

function validFunction(): int {
    return 42;
}

// 错误1: 缺少分号的表达式语句
echo "before error"
$x = 1;  // 这里应该触发错误恢复

// 错误2: 不完整的赋值表达式
$y = ;   // 缺少右操作数
$z = 3;  // 应该能继续解析

// 错误3: 非法的token序列
function 123invalid(): void {  // 函数名不能以数字开头
    return;
}

// 错误4: 括号不匹配
if ($x == 1 {
    echo "unmatched brace";
}

// 正确代码应该仍然能被解析
function anotherValidFunction(int $a, int $b): int {
    return $a + $b;
}

$result = anotherValidFunction(10, 20);
echo $result;  // 应该输出 30

?>