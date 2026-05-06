<?php
declare(strict_types=1);

function add(int $a, int $b): int {
    $result = $a + $b;
    return $result;
}

function loopSum(int $n): int {
    $sum = 0;
    for ($i = 1; $i <= $n; $i = $i + 1) {
        $sum = $sum + $i;
    }
    return $sum;
}

echo add(3, 5);
echo loopSum(10);
