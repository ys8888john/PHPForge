<?php
declare(strict_types=1);

function add(int $lhs, int $rhs): int {
    return $lhs + $rhs;
}

$ret = add(1, 1);
echo $undefined_var;  // 应该报错：未定义变量
?>
