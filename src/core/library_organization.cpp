// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/library_organization.hpp"
#include "xiphos/core/editable_document.hpp"
#include "xml_document.hpp"
#include <glib.h>
#include <filesystem>
#include <memory>
#include <set>
#include <stdexcept>
namespace xiphos::core {
namespace {
void invalid(){throw std::invalid_argument("Invalid library organization schema, identity or bounds");}
std::string property(xmlNode *node,const char *key){std::unique_ptr<xmlChar,decltype(xmlFree)> value(xmlGetProp(node,BAD_CAST key),xmlFree);return value?reinterpret_cast<const char *>(value.get()):"";}
void field(const std::string &text,std::size_t maximum,bool required=false){
 if((required&&text.empty())||text.size()>maximum||!editable_document_text(text))invalid();
 detail::validate_xml_text(text,maximum);for(unsigned char c:text)if(c<32||c==127)invalid();
}
void validate(const LibraryOrganizationEntry &entry){field(entry.identity.provider,256,true);field(entry.identity.item,4096,true);field(entry.group,256);}
bool flag(xmlNode *node,const char *key){auto value=property(node,key);if(value.empty()||value=="no")return false;if(value=="yes")return true;invalid();return false;}
std::string digest(const std::string &text){std::unique_ptr<gchar,decltype(&g_free)> p(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(text.data()),text.size()),g_free);if(!p)throw std::bad_alloc();return p.get();}
detail::XmlDocument document(const std::string &xml){if(xml.size()>maximum_editable_document_bytes)invalid();auto doc=detail::parse_xml_document(xml,"XiphosLibrary");auto *root=xmlDocGetRootElement(doc.get());if(root->ns||property(root,"version")!="1")invalid();return doc;}
void put(xmlNode *node,const char *name,const std::string &value){if(!xmlSetProp(node,BAD_CAST name,BAD_CAST value.c_str()))throw std::bad_alloc();}
}
LibraryOrganizationSnapshot parse_library_organization(const std::string &xml){
 auto doc=document(xml);LibraryOrganizationSnapshot result;result.revision=digest(xml);
 std::set<std::pair<std::string,std::string>> identities;
 for(auto *node=xmlDocGetRootElement(doc.get())->children;node;node=node->next){
  if(node->type!=XML_ELEMENT_NODE)continue;
  if(node->ns||xmlStrcmp(node->name,BAD_CAST "Item")){++result.ignored;continue;}
  LibraryOrganizationEntry entry{{property(node,"provider"),property(node,"id")},flag(node,"favorite"),flag(node,"hidden"),property(node,"group")};
  validate(entry);if(result.entries.size()==10000||!identities.emplace(entry.identity.provider,entry.identity.item).second)invalid();
  result.entries.push_back(std::move(entry));
 }
 return result;
}
LibraryOrganizationSnapshot read_library_organization(const std::string &path){return parse_library_organization(read_editable_document(path).text);}
LibraryOrganizationSnapshot set_library_organization(const std::string &path,const std::optional<std::string> &revision,const LibraryOrganizationEntry &entry){
 validate(entry);std::string source="<XiphosLibrary version=\"1\"/>";
 if(revision){if(!editable_document_revision(*revision))invalid();auto current=read_editable_document(path);if(current.revision!=*revision)throw std::invalid_argument("Library preferences changed; reload before editing");source=std::move(current.text);}
 else if(std::filesystem::exists(std::filesystem::u8path(path)))throw std::invalid_argument("Library preferences already exist");
 (void)parse_library_organization(source);auto doc=document(source);auto *root=xmlDocGetRootElement(doc.get());xmlNode *target=nullptr;
 for(auto *node=root->children;node;node=node->next)if(node->type==XML_ELEMENT_NODE&&!node->ns&&!xmlStrcmp(node->name,BAD_CAST "Item")&&property(node,"provider")==entry.identity.provider&&property(node,"id")==entry.identity.item){target=node;break;}
 if(!target){target=xmlNewChild(root,nullptr,BAD_CAST "Item",nullptr);if(!target)throw std::bad_alloc();}
 put(target,"provider",entry.identity.provider);put(target,"id",entry.identity.item);put(target,"favorite",entry.favorite?"yes":"no");put(target,"hidden",entry.hidden?"yes":"no");put(target,"group",entry.group);
 xmlChar *buffer=nullptr;int length=0;xmlDocDumpFormatMemoryEnc(doc.get(),&buffer,&length,"UTF-8",1);std::unique_ptr<xmlChar,decltype(xmlFree)> owned(buffer,xmlFree);if(!buffer||length<0)throw std::bad_alloc();
 const std::string serialized(reinterpret_cast<const char *>(buffer),length);auto prepared=parse_library_organization(serialized);
 (void)save_editable_document(path,revision,serialized);return prepared;
}
}
