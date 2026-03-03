<?php
declare(strict_types=1);

function add(int $lhs, int $rhs): int {
    return $lhs + $rhs;
}

$ret = add(1, 1);
echo $ret;  // shoud print 2
?>