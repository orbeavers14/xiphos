std::vector<std::string> actual_commentary_positions(sword::SWModule &imodule) {
using sword::VerseKey; using sword::SWKey;
imodule.setSkipConsecutiveLinks(true);
VerseKey *key = (VerseKey *)(SWKey *) imodule;
int curChapter = key->getChapter();
int curBook = key->getBook();
std::vector<std::string> result;
for (key->setVerse(1);
	     (key->getBook() == curBook) && (key->getChapter() == curChapter) && !imodule.popError();
	     imodule++) {
result.push_back(key->getText());
}
return result;
}
