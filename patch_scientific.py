import sys

with open("calc.cpp", "r") as f:
    content = f.read()

start_idx = content.find("TQWidget* Calculator::setupScientificKeys_win10(TQWidget *parent)")
end_idx = content.find("void Calculator::slotSciAngleClicked(void)", start_idx)

original_method = content[start_idx:end_idx]

# Modifications:
new_method = original_method.replace(
    "TQGridLayout *grid = new TQGridLayout(mScientificPageGrid, 7, 5, 0, mInternalSpacing);",
    "mScientificPageGrid->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);\n\tTQGridLayout *grid = new TQGridLayout(mScientificPageGrid, 7, 5, 0, 2);"
)

# Add SciNumButtonGroup creation
new_method = new_method.replace(
    "mScientificPage = new TQWidget(parent);",
    "mScientificPage = new TQWidget(parent);\n\tSciNumButtonGroup = new TQButtonGroup(this, \"Sci-Num-Button-Group\");\n\tconnect(SciNumButtonGroup, TQ_SIGNAL(clicked(int)), TQ_SLOT(slotNumberclicked(int)));"
)

# Insert digits into SciNumButtonGroup
for i in range(10):
    new_method = new_method.replace(
        f"CalcButton *btnSci{i} = new CalcButton(\"{i}\"",
        f"CalcButton *btnSci{i} = new CalcButton(\"{i}\""
    )
    # Actually, we can just append the inserts at the end before "return mScientificPageGrid;"
    
inserts = "\n\t".join([f"SciNumButtonGroup->insert(btnSci{i}, {i});" for i in range(10)])

# Loop for size policies
size_policy_loop = """
	if (TQObjectList *l = mScientificPageGrid->children()) {
		TQObjectListIt it(*l);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			if (obj->inherits("CalcButton")) {
				((CalcButton*)obj)->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
			}
		}
	}
"""

new_method = new_method.replace(
    "\tmScientificPage->hide();",
    f"\t{inserts}\n{size_policy_loop}\n\tmScientificPage->hide();"
)

content = content[:start_idx] + new_method + content[end_idx:]

with open("calc.cpp", "w") as f:
    f.write(content)

