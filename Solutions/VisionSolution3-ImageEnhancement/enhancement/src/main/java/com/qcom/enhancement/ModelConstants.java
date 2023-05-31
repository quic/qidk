package com.qcom.enhancement;

public class ModelConstants {
    private final String modelName;
    private final boolean normalised_input;
    private final boolean encoding_rgb;
    private final int inputH;
    private final int inputW;
    private final int outputH;
    private final int outputW;

    ModelConstants(String modelName,
                   int inputH, int inputW, int outputH, int outputW,
                   boolean normalised_input, boolean encoding_rgb) {
        this.modelName = modelName;
        this.inputH = inputH;
        this.inputW = inputW;
        this.outputH = outputH;
        this.outputW = outputW;
        this.normalised_input = normalised_input;
        this.encoding_rgb = encoding_rgb;

    }

    public String getModelName() {
        return modelName;
    }
    public boolean doNormaliseInput() {
        return normalised_input;
    }
    public boolean isEncodingRGB() {
        return encoding_rgb;
    }
    public int getInputHeight() {
        return inputH;
    }
    public int getInputWidth() {
        return inputW;
    }
    public int getOutputHeight() {
        return outputH;
    }
    public int getOutputWidth() {
        return outputW;
    }
}
