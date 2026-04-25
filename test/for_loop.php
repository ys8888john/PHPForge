<?php
declare(strict_types=1);

function sum(int $n): int {
    $result = 0;
    for ($i = 1; $i <= $n; $i = $i + 1 + 1 - 1) {
        $result = $result + $i;
    }
    return $result;
}

echo sum(10);
