#include "plan.h"
#include <iostream>
#include <vector>
#include "table.h"
#include "hashjoin.h"
#include "ripplejoin.h"
#include "timer.h"
#include "base.h"
#include <cstring>
using namespace std;
static size_t max_time = 1000000ull * 60 * 60;
static size_t step_time = 100000;
void test_dynamic(){
	const size_t n_rels = 3;
	//dynamic_method
	base_table *p[n_rels];
	p[0] = new table_customer();
	p[1] = new table_orders();
	p[2] = new table_lineitem();
	plan_node node[n_rels];
	for(size_t i = 0; i < n_rels; ++i){
		p[i]->load_data();
		p[i]->build_hash();
		//node[i].p_head = &head_node[i]; 
	}
	//! here to define cond function
	std::array<head_node_type, n_rels> head_node;
	std::vector<date_t> cond_date{{1998,12,1}, {1998,6,1}, {1997,12,1}, {1997,6,1},\
		{1996,12,1}, {1996, 6, 1},{1995,12,1}, {1995,6,1}, {1994,12,1}, {1994,6,1}, {1993,12,1}, {1993, 6,1},\
		{1992,12,1}};
	for(auto &x:cond_date){
		date_t cond_date1{1992,1,1};
		date_t cond_date2 = x;
	std::function<bool(base_raw *)> cond_func_customer= [&](base_raw *input)->bool{\
			if(input->raw_tag == RAW_CUSTOMER){
				auto p = (raw_customer*)input;
				if(std::strcmp(p->c_mktsegment, "BUILDING") == 0)
					return true;
			}
			return false;
		};
	std::function<bool(base_raw *)> cond_func_orders = [&](base_raw *input)->bool{\
			if(input->raw_tag == RAW_ORDERS){
				auto p = (raw_orders*) input;
				if(p->o_orderdate < cond_date2){
					return true;
				}
			}
			return false;
		};
	std::function<bool(base_raw *)> cond_func_lineitem = [&](base_raw *input)->bool{\
	return true;
			if(input->raw_tag == RAW_LINEITEM){
				auto p = (raw_lineitem*) input;
				if(cond_date2 < p->l_shipdate){
					return true;
				}
			}
			return false;
		};
	head_node[0].col_id = 3;
	head_node[0].segment = "BUILDING";
	head_node[1].col_id = 3;
	head_node[1].start_date = date_t{1992,1,1};
	head_node[1].end_date = cond_date1;
	head_node[2].col_id = 5;
	head_node[2].start_date = cond_date2;
	head_node[2].end_date = date_t{9999,9,9};
	for(size_t i = 0; i < n_rels; ++i){
		node[i].cond_func = nullptr;
		node[i].p_head = nullptr; 
	}
	//node[0].p_head = &head_node[0];
	node[1].p_head = &head_node[1];
	//node[0].cond_func = &cond_func_customer;
	//node[1].cond_func = &cond_func_orders;
	node[1].cond_func = &cond_func_orders;
	//! here to define the head node

	//hash_join
	std::vector<base_raw *> vec_c_p;
	std::vector<base_raw *> vec_o_p;
	std::vector<base_raw *> vec_l_p;
	auto p0 = dynamic_cast<table_customer*>(p[0]);
	auto p1 = dynamic_cast<table_orders *>(p[1]);
	auto p2 = dynamic_cast<table_lineitem *>(p[2]);
	for(auto &x:p0->data)
		vec_c_p.push_back(&x);
	for(auto &x:p1->data)
		vec_o_p.push_back(&x);
	for(auto &x:p2->data)
		vec_l_p.push_back(&x);
	//! hash join no cond
	size_t fullsize = 0;
	{
	timer mytimer;
	auto a = hash_join(vec_c_p, vec_o_p, nullptr, nullptr);
	auto b = hash_join(a, vec_l_p, nullptr, nullptr);
	double count = 0;
	for(auto &x:b){
		auto tmp = dynamic_cast<raw_lineitem* >(x);
		count += tmp->l_extendedprice * (1.0 - tmp->l_discount);
	}
	fullsize = b.size();
	std::cout<<"size: "<<fullsize<<"\tresult: "<<count << "\trunning time is: "<< mytimer.get_elapsed()<<endl;
	}
	//! hash join with cond
	double value = 0;
	{
	timer mytimer;
	auto a = hash_join(vec_c_p, vec_o_p, nullptr, nullptr);
	auto b = hash_join(a, vec_l_p, &cond_func_orders, nullptr);
	double count = 0;
	for(auto &x:b){
		auto tmp = dynamic_cast<raw_lineitem* >(x);
		count += tmp->l_extendedprice * (1.0 - tmp->l_discount);
	}
	value = count;
	std::cout<<"Selectivity: "<<1.0-b.size()*1.0/fullsize<<"\tresult: "<<count << "\trunning time is: "<< mytimer.get_elapsed()<<endl;
	}
	{
		typedef std::function<uint32_t(base_raw*)> func_type;
		std::vector<size_t> a(3);
		auto  &vec_c = p0->data;
		auto &vec_o = p1->data;
		auto &vec_l = p2->data;
		double total_size = vec_c.size() * vec_o.size() * vec_l.size();
		a[2] = vec_l.size() / vec_c.size();
		a[1] = vec_o.size() / vec_c.size();
		a[0] = 1;
		//a[2] = 1;
		//a[1] = 1;
		std::vector<base_raw *> vec_c_p;
		std::vector<base_raw *> vec_o_p;
		std::vector<base_raw *> vec_l_p;
		for(auto &x:vec_c)
			vec_c_p.push_back(&x);
		for(auto &x:vec_o)
			vec_o_p.push_back(&x);
		for(auto &x:vec_l)
			vec_l_p.push_back(&x);
		std::vector<std::vector<base_raw*>> table;
		table.push_back(vec_c_p);
		table.push_back(vec_o_p);
		table.push_back(vec_l_p);
		std::vector<cond_func_type> cond_func(3);
		std::vector<std::vector<key_func_type>> key_func;
		for(size_t i = 0; i < 3; ++i){
			cond_func[i] = nullptr;
		}
		//cond_func[0] = &cond_func_customer;
		cond_func[1] = &cond_func_orders;
		//cond_func[2] = &cond_func_lineitem;
		func_type customer_orders = [&](base_raw *input)->uint32_t{\
			auto tmp = dynamic_cast<raw_customer *>(input);
			return tmp->c_custkey;
			};
		func_type orders_customer = [&](base_raw *input)->uint32_t{\
			auto tmp = dynamic_cast<raw_orders *>(input);
			return tmp->o_custkey;
			};
		func_type orders_lineitem = [&](base_raw *input)->uint32_t{\
			auto tmp = dynamic_cast<raw_orders *>(input);
			return tmp->o_orderkey;
			};
		func_type lineitem_orders = [&](base_raw *input)->uint32_t{\
			auto tmp = dynamic_cast<raw_lineitem *>(input);
			return tmp->l_orderkey;
			};
		for(size_t i = 0; i < 3; ++i)
		{
			std::vector<key_func_type> tmp;
			key_func.push_back(tmp);
			for(size_t j = 0; j < 3; ++j)
				key_func[i].push_back(nullptr);
		}
		key_func[0][1] = &customer_orders;
		key_func[1][0] = &orders_customer;
		key_func[1][2] = &orders_lineitem;
		key_func[2][1] = &lineitem_orders;
		std::function<double(base_raw*, base_raw*, base_raw*)> result_func =[&](base_raw* t1, base_raw *t2, base_raw*t3)->double{
			raw_lineitem *q = (raw_lineitem *)t3;
			double result = q->l_extendedprice * (1 - q->l_discount);
			return result;
		};
		ripple_3_ratio_test(table, a, key_func, cond_func, result_func, step_time, max_time, .95, true, value*0.01, total_size);
		index_ripple_query10(p0->data, p1->data, p2->data, &cond_date1, &cond_date2, nullptr, key_func, result_func, step_time, max_time, .95, value*0.01, true);
	}
		}
	for(auto &x:cond_date){
		date_t cond_date1{1992,1,1};
		date_t cond_date2 = x;
	std::function<bool(base_raw *)> cond_func_customer= [&](base_raw *input)->bool{\
			if(input->raw_tag == RAW_CUSTOMER){
				auto p = (raw_customer*)input;
				if(std::strcmp(p->c_mktsegment, "BUILDING") == 0)
					return true;
			}
			return false;
		};
	std::function<bool(base_raw *)> cond_func_orders = [&](base_raw *input)->bool{\
			if(input->raw_tag == RAW_ORDERS){
				auto p = (raw_orders*) input;
				if(p->o_orderdate < cond_date2){
					return true;
				}
			}
			return false;
		};
	head_node[0].col_id = 3;
	head_node[0].segment = "BUILDING";
	head_node[1].col_id = 3;
	head_node[1].start_date = date_t{1992,1,1};
	head_node[1].end_date = cond_date2;
	head_node[2].col_id = 5;
	head_node[2].start_date = cond_date2;
	head_node[2].end_date = date_t{9999,9,9};
	for(size_t i = 0; i < n_rels; ++i){
		node[i].cond_func = nullptr;
		node[i].p_head = nullptr; 
	}
	node[0].p_head = &head_node[0];
	node[1].p_head = &head_node[1];
	node[0].cond_func = &cond_func_customer;
	//node[1].cond_func = &cond_func_orders;
	node[1].cond_func = &cond_func_orders;
	//! here to define the head node

	//hash_join
	std::vector<base_raw *> vec_c_p;
	std::vector<base_raw *> vec_o_p;
	std::vector<base_raw *> vec_l_p;
	auto p0 = dynamic_cast<table_customer*>(p[0]);
	auto p1 = dynamic_cast<table_orders *>(p[1]);
	auto p2 = dynamic_cast<table_lineitem *>(p[2]);
	for(auto &x:p0->data)
		vec_c_p.push_back(&x);
	for(auto &x:p1->data)
		vec_o_p.push_back(&x);
	for(auto &x:p2->data)
		vec_l_p.push_back(&x);
	//! hash join no cond
	size_t fullsize = 0;
	{
	timer mytimer;
	auto a = hash_join(vec_c_p, vec_o_p, nullptr, nullptr);
	auto b = hash_join(a, vec_l_p, nullptr, nullptr);
	double count = 0;
	for(auto &x:b){
		auto tmp = dynamic_cast<raw_lineitem* >(x);
		count += tmp->l_extendedprice * (1.0 - tmp->l_discount);
	}
	fullsize = b.size();
	std::cout<<"size: "<<fullsize<<"\tresult: "<<count << "\trunning time is: "<< mytimer.get_elapsed()<<endl;
	}
	//! hash join with cond
	double value = 0;
	{
	timer mytimer;
	auto a = hash_join(vec_c_p, vec_o_p, &cond_func_customer, nullptr);
	auto b = hash_join(a, vec_l_p, &cond_func_orders, nullptr);
	double count = 0;
	for(auto &x:b){
		auto tmp = dynamic_cast<raw_lineitem* >(x);
		count += tmp->l_extendedprice * (1.0 - tmp->l_discount);
	}
	value = count;
	std::cout<<"Selectivity: "<<1.0-b.size()*1.0/fullsize<<"\tresult: "<<count << "\trunning time is: "<< mytimer.get_elapsed()<<endl;
	}
	{
		typedef std::function<uint32_t(base_raw*)> func_type;
		std::vector<size_t> a(3);
		auto  &vec_c = p0->data;
		auto &vec_o = p1->data;
		auto &vec_l = p2->data;
		double total_size = vec_c.size() * vec_o.size() * vec_l.size();
		a[2] = vec_l.size() / vec_c.size();
		a[1] = vec_o.size() / vec_c.size();
		a[0] = 1;
		//a[2] = 1;
		//a[1] = 1;
		std::vector<base_raw *> vec_c_p;
		std::vector<base_raw *> vec_o_p;
		std::vector<base_raw *> vec_l_p;
		for(auto &x:vec_c)
			vec_c_p.push_back(&x);
		for(auto &x:vec_o)
			vec_o_p.push_back(&x);
		for(auto &x:vec_l)
			vec_l_p.push_back(&x);
		std::vector<std::vector<base_raw*>> table;
		table.push_back(vec_c_p);
		table.push_back(vec_o_p);
		table.push_back(vec_l_p);
		std::vector<cond_func_type> cond_func(3);
		std::vector<std::vector<key_func_type>> key_func;
		for(size_t i = 0; i < 3; ++i){
			cond_func[i] = nullptr;
		}
		cond_func[0] = &cond_func_customer;
		cond_func[1] = &cond_func_orders;
		//cond_func[2] = &cond_func_lineitem;
		func_type customer_orders = [&](base_raw *input)->uint32_t{\
			auto tmp = dynamic_cast<raw_customer *>(input);
			return tmp->c_custkey;
			};
		func_type orders_customer = [&](base_raw *input)->uint32_t{\
			auto tmp = dynamic_cast<raw_orders *>(input);
			return tmp->o_custkey;
			};
		func_type orders_lineitem = [&](base_raw *input)->uint32_t{\
			auto tmp = dynamic_cast<raw_orders *>(input);
			return tmp->o_orderkey;
			};
		func_type lineitem_orders = [&](base_raw *input)->uint32_t{\
			auto tmp = dynamic_cast<raw_lineitem *>(input);
			return tmp->l_orderkey;
			};
		for(size_t i = 0; i < 3; ++i)
		{
			std::vector<key_func_type> tmp;
			key_func.push_back(tmp);
			for(size_t j = 0; j < 3; ++j)
				key_func[i].push_back(nullptr);
		}
		key_func[0][1] = &customer_orders;
		key_func[1][0] = &orders_customer;
		key_func[1][2] = &orders_lineitem;
		key_func[2][1] = &lineitem_orders;
		std::function<double(base_raw*, base_raw*, base_raw*)> result_func =[&](base_raw* t1, base_raw *t2, base_raw*t3)->double{
			raw_lineitem *q = (raw_lineitem *)t3;
			double result = q->l_extendedprice * (1 - q->l_discount);
			return result;
		};
		ripple_3_ratio_test(table, a, key_func, cond_func, result_func, step_time, max_time, .95, true, value*0.01, total_size);
		unsigned char flag = 'R';
		index_ripple_query10(p0->data, p1->data, p2->data, &cond_date1, &cond_date2, &flag, key_func, result_func, step_time, max_time, .95, value*0.01, true);
	}
		}
	for(size_t i = 0; i < n_rels; ++i){
		delete p[i];
	}
	return;
}
int main(int argc, char* argv[]){
	test_dynamic();
	return 0;
}