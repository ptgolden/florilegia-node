#include <string>
#include <list>
#include <poppler.h>

using std::string;
using std::list;

enum AnnotationAction {
	ANNOTATION_ACTION_UNSUPPORTED,

	ANNOTATION_ACTION_COMMENTING,
	ANNOTATION_ACTION_HIGHLIGHTING,
	ANNOTATION_ACTION_STAMPING
};

struct OacAnnot {
	int page;
	AnnotationAction action;

	string body_text;
	string body_image;
	string target_text;
};

string annotation_action_get_motivation(AnnotationAction action);

void oac_annot_set_annotation_action(OacAnnot *oac_annot, PopplerAnnot *annot);

void oac_annot_set_body_text(OacAnnot *oac_annot, PopplerAnnot *annot);

void oac_annot_set_body_image(OacAnnot *oac_annot, PopplerAnnot *annot);

void oac_annot_set_target_text(OacAnnot *oac_annot, PopplerAnnot *annot);

string oac_annot_to_json(OacAnnot *oac_annot);

void oac_annot_from_mapping(OacAnnot *oac_annot, PopplerAnnotMapping *m, PopplerPage *p);

list<OacAnnot> poppler_document_get_oac_annots(PopplerDocument *doc, int page_number);
