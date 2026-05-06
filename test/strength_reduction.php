<?php
declare(strict_types=1);

function mul4(int $x): int {
    return $x * 4;
}

function mul_by_zero_one(int $x): int {
    $a = $x * 0;  // 0 < 2，不替换
    $b = $x * 1;  // 1 < 2，不替换
    return $a + $b;
}

echo mul4(5);
echo mul_by_zero_one(10);
