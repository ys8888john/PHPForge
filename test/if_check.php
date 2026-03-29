<?php
declare(strict_types=1);

function check(bool $status): string {
    if ($status == true) {
        return '$status is: true';
    }
    return '$status is: false';

    echo 'should not reach here';
}

$status = true;
$ret = check($status);
echo $ret;  // shoud print $status is: true

$status = false;
$ret = check($status);
echo $ret;  // shoud print $status is: false
?>