

fn f() {
    const b = array(1,2,3);
    let t = 0;
    let k = b[t];
}

@compute @workgroup_size(1) fn main(){
    f();
}