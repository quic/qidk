# Check if SDK_SETUP_ENV is set or not for checking whether environment is set or not.
# Exit compilation if SDK_SETUP_ENV is not set.

ifndef SDK_SETUP_ENV
$(error Error! SDK Environment not set up -> please run setup_sdk_env script from SDK root directory.)
endif

include $(HEXAGON_SDK_ROOT)/build/make.d/utils.min

# include the variant specific .min files
# V = hexagon --> hexagon.min
# V = android --> android.min

ifndef V
$(error Variant must be provided, pass a variant by adding 'V=<desired Variant>' to your build command)
endif

V_TARGET = $(word 1,$(subst _, ,$(V)))
ifeq ($(wildcard $(V_TARGET)_deps.min $(V_TARGET)_rules.min), )
$(error Unsupported target '$(V)' in variant '$(V)', check that both $(V_TARGET)_deps.min and $(V_TARGET)_rules.min exist)
endif

include $(V_TARGET)_deps.min
include $(HEXAGON_SDK_ROOT)/build/make.d/$(V_TARGET)_vs.min
include $(HEXAGON_SDK_ROOT)/build/defines.min
include $(V_TARGET).min


# always last
include $(RULES_MIN)
