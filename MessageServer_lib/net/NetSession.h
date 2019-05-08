#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>

namespace mserver {
	namespace net {

		class NetSession
		{
		private:
			struct UserDataClass {};

			struct AbstractUserData
			{
				virtual ~AbstractUserData() {}
				virtual const UserDataClass* get_class() = 0;
			};

			template<typename T> struct UserData : AbstractUserData
			{
				T data_;
				static const UserDataClass class_;

				virtual const UserDataClass* get_class() override { return &class_; }
			};

			std::string client_id_;
			std::unordered_map<std::string, std::unique_ptr<AbstractUserData>> table_;
			
		public:
			explicit NetSession(std::string client_id)
				: client_id_(std::move(client_id))
			{}

			const std::string& client_id() const
			{
				return client_id_;
			}

			template<typename T> T& get(const std::string& key)
			{
				auto& p = table_[key];
				if (p)
				{
					if (p->get_class() != &UserData<T>::class_)
						throw std::runtime_error("mismatched user data type.");
				}
				else
				{
					p = std::make_unique<UserData<T>>();
				}

				return static_cast<UserData<T>*>(p.get())->data_;
			}

			void erase(const std::string& key)
			{
				table_.erase(key);
			}
		};

		template<typename T> const NetSession::UserDataClass NetSession::UserData<T>::class_;
	}
}