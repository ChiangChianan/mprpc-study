#include <iostream>
#include "test.pb.h"

int main() {
  proto_test::LoginResponse rep;
  proto_test::ResultCode *rep_rc = rep.mutable_result_code();
  rep_rc->set_errcode(1);
  rep_rc->set_errmsg("登录处理失败");
  rep.set_success(true);

  proto_test::GetFriendListResponse rsp;
  proto_test::ResultCode *rsp_rc = rsp.mutable_result_code();
  proto_test::User *usr1 = rsp.add_friend_list();

  usr1->set_age(15);
  usr1->set_gender(proto_test::MALE);
  usr1->set_name("张三");

  proto_test::User *usr2 = rsp.add_friend_list();
  usr2->set_age(16);
  usr2->set_gender(proto_test::MALE);
  usr2->set_name("李四");

  std::string repstr;
  if (rep.SerializeToString(&repstr)) {
    std::cout << "DebugString输出:" << std::endl;
    std::cout << rep.DebugString() << std::endl;
  }

  proto_test::LoginResponse rep2;
  rep2.ParseFromString(repstr);
  std::cout << rep2.success() << std::endl;
  proto_test::ResultCode result_code = rep2.result_code();
  std::cout << result_code.errcode() << std::endl;
  std::cout << result_code.errmsg() << std::endl;

  return 0;
}