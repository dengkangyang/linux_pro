#include "tinyxml2.h"
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h>
using namespace tinyxml2;
using namespace std;


class User
{
public:
	User(){
		gender = 0;
	};

	User(const string& userName, const string& password, int gender, const string& mobile, const string& email){
		this->userName = userName;
		this->password = password;
		this->gender = gender;
		this->mobile = mobile;
		this->email = email;
	};

	string userName;
	string password;
	int gender;
	string mobile;
	string email;
};



int CreatXML(const char* szFileName)
{
	const char* declaration = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
	XMLDocument doc;
	doc.Parse(declaration);

	//�������
	XMLElement* root = doc.NewElement("DBUSER");
	doc.InsertEndChild(root);

	return doc.SaveFile(szFileName);
}

int insertXMLNode(const char* xmlPath, const User& usr)
{
	XMLDocument doc;
	int res = doc.LoadFile(xmlPath);
	if (0 != res)
	{
		cout << "load xml file failed!" << endl;
		return res;
	}

	XMLElement* root = doc.RootElement();

	XMLElement* usrNode = doc.NewElement("User");
	usrNode->SetAttribute("Name", usr.userName.c_str());
	usrNode->SetAttribute("Password", usr.password.c_str());
	root->InsertEndChild(usrNode);

	XMLElement* gender = doc.NewElement("Gender");
	char szTemp[20] = { 0 };
	sprintf(szTemp, "%d", usr.gender);
	XMLText* genderText = doc.NewText(szTemp);
	gender->InsertEndChild(genderText);
	usrNode->InsertEndChild(gender);

	XMLElement* mobile = doc.NewElement("Mobile");
	mobile->InsertEndChild(doc.NewText(usr.mobile.c_str()));
	usrNode->InsertEndChild(mobile);

	XMLElement* email = doc.NewElement("Email");
	email->InsertEndChild(doc.NewText(usr.email.c_str()));
	usrNode->InsertEndChild(email);

	return doc.SaveFile(xmlPath);

}

//function:�����û�����ȡ�û��ڵ�
//param:root:xml�ļ����ڵ㣻userName���û���
//return���û��ڵ�
XMLElement* queryUserNodeByName(XMLElement* root, const string& userName)
{

	XMLElement* userNode = root->FirstChildElement("User");
	while (userNode != NULL)
	{
		if (userNode->Attribute("Name") == userName)
			break;
		userNode = userNode->NextSiblingElement();//��һ���ֵܽڵ�
	}
	return userNode;
}

User* queryUserByName(const char* xmlPath, const string& userName)
{
	XMLDocument doc;
	if (doc.LoadFile(xmlPath) != 0)
	{
		cout << "load xml file failed" << endl;
		return NULL;
	}
	XMLElement* root = doc.RootElement();
	XMLElement* userNode = queryUserNodeByName(root, userName);

	if (userNode != NULL)  //searched successfully
	{
		User* user = new User();
		user->userName = userName;
		user->password = userNode->Attribute("Password");
		XMLElement* genderNode = userNode->FirstChildElement("Gender");
		user->gender = atoi(genderNode->GetText());
		XMLElement* mobileNode = userNode->FirstChildElement("Mobile");
		user->mobile = mobileNode->GetText();
		XMLElement* emailNode = userNode->FirstChildElement("Email");
		user->email = emailNode->GetText();
		return user;
	}
	return NULL;
}


bool updateUser(const char* xmlPath, User* user)
{
	XMLDocument doc;
	if (doc.LoadFile(xmlPath) != 0)
	{
		cout << "load xml file failed" << endl;
		return false;
	}
	XMLElement* root = doc.RootElement();
	XMLElement* userNode = queryUserNodeByName(root, user->userName);

	if (userNode != NULL)
	{
		if (user->password != userNode->Attribute("Password"))
		{
			userNode->SetAttribute("Password", user->password.c_str());  //�޸�����
		}
		XMLElement* genderNode = userNode->FirstChildElement("Gender");
		if (user->gender != atoi(genderNode->GetText()))
		{
			genderNode->SetText(itoa(user->gender).c_str());   //�޸Ľڵ�����
		}
		XMLElement* mobileNode = userNode->FirstChildElement("Mobile");
		if (user->mobile != mobileNode->GetText())
		{
			mobileNode->SetText(user->mobile.c_str());
		}
		XMLElement* emailNode = userNode->FirstChildElement("Email");
		if (user->email != emailNode->GetText())
		{
			emailNode->SetText(user->email.c_str());
		}
		if (doc.SaveFile(xmlPath) == 0)
			return true;
	}
	return false;
}


//function:ɾ��ָ���ڵ�����
//param:xmlPath:xml�ļ�·����userName���û�����
//return��bool
bool deleteUserByName(const char* xmlPath, const string& userName)
{
	XMLDocument doc;
	if (doc.LoadFile(xmlPath) != 0)
	{
		cout << "load xml file failed" << endl;
		return false;
	}
	XMLElement* root = doc.RootElement();
	//doc.DeleteNode(root);//ɾ��xml���нڵ�
	XMLElement* userNode = queryUserNodeByName(root, userName);
	if (userNode != NULL)
	{
		userNode->DeleteAttribute("Password");//ɾ������
		XMLElement* emailNode = userNode->FirstChildElement("Email");
		userNode->DeleteChild(emailNode); //ɾ��ָ���ڵ�
		//userNode->DeleteChildren();//ɾ���ڵ�����к��ӽڵ�
		if (doc.SaveFile(xmlPath) == 0)
			return true;
	}
	return false;
}

int main()
{
	CreatXML("test.xml");
	User aa("dd", "1234", 12, "2123", "kka@main.cm");
	insertXMLNode("test.xml", aa );

	User* pUser = queryUserByName("test.xml", "dd");
	std::cout << pUser->userName << "\t"
		<< pUser->gender << "\t"
		<< pUser->password << "\t"
		<< pUser->mobile << "\t"
		<< pUser->email << endl;


	return 0;
}