#include <iostream>
#include "test.pb.h"

int main() {
  proto_test::LoginResponse rep;
  proto_test::ResultCode* rep_rc = rep.mutable_result_code();
  rep_rc->set_errcode(1);
  rep_rc->set_errmsg("登录处理失败");

  proto_test::GetFriendListResponse rsp;
  proto_test::ResultCode* rsp_rc = rep.mutable_result_code();
  proto_test::User* usr1 = rsp.add_friend_list();

  usr1->set_age(15);
  usr1->set_gender(proto_test::GENDER_MALE);
  usr1->set_name("张三");

  proto_test::User* usr2 = rsp.add_friend_list();
  usr2->set_age(16);
  usr2->set_gender(proto_test::GENDER_MALE);
  usr2->set_name("李四");

  std::cout << rsp.friend_list_size() << std::endl;
  return 0;
}