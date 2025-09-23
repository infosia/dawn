

fn Func(a:u32){

    var x = 222;
    switch a {
    case 1: {
      x= 1;
    }
    // Multiple selectors for one block
    case 2, 3,8: {
          x= 3;
    }
    // Lone default
    default: {
      x = 44;
    }
  }

}

@compute
@workgroup_size(1)
fn main(){
    Func(3);
}
